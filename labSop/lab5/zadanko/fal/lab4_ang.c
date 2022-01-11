#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <aio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <ftw.h>
#include <dirent.h>
#include <pthread.h>
#define BLOCKS 3
#define BLOCK_SIZE 4 // 128
#define SHIFT(counter, x) ((counter + x) % BLOCKS)
#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))


void usage(char*);
int Suspend(struct aiocb*);
void ReadData(struct aiocb*, off_t);
void WriteData(struct aiocb*, off_t);
//void SyncData(struct aiocb*);
//void Cleanup(char**, int);
void ReadArguments(int , char**, char**, char** , char**, int*);
void SetUp(char* , char* , char*, int, char **, struct aiocb *);
void Work(struct aiocb *);
void ProccessBlock(char *, char *, char *, int n);

void usage(char* progName)
{
    fprintf(stderr, "%s INPUT_FILE MASK_FILE OUTPUT_FILE PARALLELISM \n", progName);
    exit(EXIT_FAILURE);
}

void SetHandler(void (*f)(int), int sig){
        struct sigaction sa;
        memset(&sa, 0x00, sizeof(struct sigaction));
        sa.sa_handler = f;
        if (sigaction(sig, &sa, NULL) == -1)
                ERR("sigaction");
}


int Suspend(struct aiocb *aiocbs)
{
    struct aiocb *aiolist[1];
    aiolist[0] = aiocbs;
    int ret;
    //if(!work) return;
    while(aio_suspend((const struct aiocb *const *) aiolist, 1, NULL) == -1)
    {
        //if(!work) return;
        if(errno == EINTR) continue;
        ERR("aio_suspend");
    }
    if (aio_error(aiocbs) != 0)
        ERR("Suspend error");
    if ((ret = aio_return(aiocbs)) == -1)
        ERR("Return error");
    return ret;
}

void ReadData(struct aiocb *aiocbs, off_t offset)
{
    //if(!work) return;
    aiocbs->aio_offset = offset;
    if(aio_read(aiocbs) == -1)
        ERR("aio_read");
}

void WriteData(struct aiocb *aiocbs, off_t offset)
{
    //if(!work) return;
    aiocbs->aio_offset = offset;
    if(aio_write(aiocbs) == -1)
        ERR("aio_write");
}
/*
void SyncData(struct aiocb *aiocbs)
{
    //if(!work) return;
    Suspend(aiocbs);
    if(aio_fsync(O_SYNC, aiocbs) == -1)
        ERR("aio_fsync");
    Suspend(aiocbs);
}
*/
void Cleanup(struct aiocb *aiocbs)
{        
    for(int i=0; i<BLOCKS; i++)
    {    
        if(aio_cancel(aiocbs[i].aio_fildes, NULL) == -1)
            ERR("aio_cancel");
        if(TEMP_FAILURE_RETRY(fsync(aiocbs[i].aio_fildes)) == -1)
            ERR("fsync");
        free((char*) aiocbs[i].aio_buf);
        if(close(aiocbs[i].aio_fildes) != 0) 
            ERR("close");
    }
    
    
}

void ReadArguments(int argc, char** argv, char** pInputName, char** pMaskName, char** pOutputName, int* pParallelism)
{
    if(argc != 5) usage(argv[0]);
    *pInputName = argv[1];
    *pMaskName = argv[2];
    *pOutputName = argv[3];
    *pParallelism = atoi(argv[4]);    
}

void SetUp(char* inputName, char* maskName, char* outputName, int parallelism, char **buffer, struct aiocb *aiocbs)
{
    int FD[BLOCKS];
    if((FD[0] = TEMP_FAILURE_RETRY(open(inputName,O_RDONLY))) < 0) ERR("open");
    if((FD[1] = TEMP_FAILURE_RETRY(open(maskName,O_RDONLY))) < 0) ERR("open");
    if((FD[2] = TEMP_FAILURE_RETRY(open(outputName, O_WRONLY|O_CREAT|O_TRUNC|O_APPEND,0777)))<0) ERR("open");

    //work = 1;
    for(int i=0; i<BLOCKS; i++)
        if((buffer[i] = (char*) calloc(BLOCK_SIZE, sizeof(char))) == NULL)
            ERR("calloc");
    
    for(int i=0; i<BLOCKS; i++)
    {
        memset(&aiocbs[i], 0, sizeof(struct aiocb));
        aiocbs[i].aio_fildes = FD[i]; 
        aiocbs[i].aio_offset = 0;
        aiocbs[i].aio_nbytes = BLOCK_SIZE;
        aiocbs[i].aio_buf = (void*) buffer[i];
        aiocbs[i].aio_sigevent.sigev_notify = SIGEV_NONE;
    }
}

void Work(struct aiocb *aiocbs)
{
    int caret = 0, n1, n2;
    
    ReadData(&aiocbs[0], caret*BLOCK_SIZE);
    ReadData(&aiocbs[1], caret*BLOCK_SIZE);
    n1 = Suspend(&aiocbs[0]);
    n2 = Suspend(&aiocbs[1]);    
    while(n1 == BLOCK_SIZE && n2 == BLOCK_SIZE)
    {       
        Suspend(&aiocbs[2]);
        ProccessBlock((char*) aiocbs[0].aio_buf, (char*) aiocbs[1].aio_buf, (char*) aiocbs[2].aio_buf, BLOCK_SIZE);
        WriteData(&aiocbs[2], caret*BLOCK_SIZE);
        ReadData(&aiocbs[0], (caret+1)*BLOCK_SIZE);
        ReadData(&aiocbs[1], (caret+1)*BLOCK_SIZE);        
        n1 = Suspend(&aiocbs[0]);
        n2 = Suspend(&aiocbs[1]);
        caret++;    
    }
    if(n1 < n2)
    {
        ProccessBlock((char*) aiocbs[0].aio_buf, (char*) aiocbs[1].aio_buf, (char*) aiocbs[2].aio_buf, n1);
        aiocbs[2].aio_nbytes = n1;
    }
    else
    {
        ProccessBlock((char*) aiocbs[0].aio_buf, (char*) aiocbs[1].aio_buf, (char*) aiocbs[2].aio_buf, n2);
        aiocbs[2].aio_nbytes = n2;
    }    printf("Test4 %d, %d, %s\n", n1, n2, (char*) aiocbs[2].aio_buf);
    Suspend(&aiocbs[2]);
    WriteData(&aiocbs[2], caret*BLOCK_SIZE);
    Cleanup(aiocbs);
    printf("Test5\n");
}

void ProccessBlock(char * inBuf, char * maskBuf, char * outBuf, int n)
{
    for(int i=0; i< BLOCK_SIZE && i<n; i++)    
        if(maskBuf[i] == ' ')
            outBuf[i] = inBuf[i];
        else
            outBuf[i] = maskBuf[i];    
}

int main(int argc, char** argv)
{    
    char *inputFile, *maskFile, *outputFile; // Nazwa pliku
    int parallelism;
    char *buffer[BLOCKS];
    // allokacja zależna od parallelism. etap 4
    struct aiocb aiocbs[BLOCKS]; // Z jakiegoś powodu w tutorialu jest 4 

    ReadArguments(argc, argv, &inputFile, &maskFile, &outputFile, &parallelism);
    SetUp(inputFile, maskFile, outputFile, parallelism, buffer, aiocbs);
    Work(aiocbs);
    return EXIT_SUCCESS;
}