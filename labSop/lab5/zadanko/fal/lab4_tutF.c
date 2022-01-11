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
#define SHIFT(counter, x) ((counter + x) % BLOCKS)
#define KB 10 //1000 // Dla testów 
#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))


typedef struct threadArgs
{         
    pthread_t tid;
    struct aiocb *pAiocb;
    int s;
} threadArgs_t;

void usage(char*);
void SigintHandler(int);
void SetHandler(void (*) (int), int);
void FillAIOStructs(struct aiocb*, char**, int);
void Suspend(struct aiocb*);
void ReadData(struct aiocb*, off_t);
void WriteData(struct aiocb*, off_t);
void SyncData(struct aiocb*);
void Cleanup(char**, int);
void ReadArguments(int, char**, int*, char**);
void SetUp(int, char*, char **, struct aiocb *);
void Work(int, char **, struct aiocb *);
void* threadWork(void*);
void JoinThreads(threadArgs_t*, int);

volatile sig_atomic_t work;

void usage(char* progName)
{
    fprintf(stderr, "%s WORKDIR BLOCKSIZE \n", progName);
    fprintf(stderr, "workdir - path to the directory to work on\n");
    fprintf(stderr, "n - number of blocks\n");
    exit(EXIT_FAILURE);
}

void SigintHandler(int sig)
{
    work = 0;
}

void SetHandler(void (*f)(int), int sig){
        struct sigaction sa;
        memset(&sa, 0x00, sizeof(struct sigaction));
        sa.sa_handler = f;
        if (sigaction(sig, &sa, NULL) == -1)
                ERR("sigaction");
}


void Suspend(struct aiocb *aiocbs)
{
    struct aiocb *aiolist[1];
    aiolist[0] = aiocbs;
    if(!work) return;
    while(aio_suspend((const struct aiocb *const *) aiolist, 1, NULL) == -1)
    {
        if(!work) return;
        if(errno == EINTR) continue;
        ERR("aio_suspend");
    }
    if (aio_error(aiocbs) != 0)
        ERR("Suspend error");
    if (aio_return(aiocbs) == -1)
        ERR("Return error");
}

void FillAIOStructs(struct aiocb* aiocbs, char** buffer, int blocksize)
{
    for(int i=0; i<BLOCKS; i++)
    {
        memset(&aiocbs[i], 0, sizeof(struct aiocb));
        //aiocbs[i].aio_fildes = fd; 
        aiocbs[i].aio_offset = 0;
        aiocbs[i].aio_nbytes = blocksize;
        aiocbs[i].aio_buf = (void*) buffer[i];
        aiocbs[i].aio_sigevent.sigev_notify = SIGEV_NONE;
    }
}

void ReadData(struct aiocb *aiocbs, off_t offset)
{
    if(!work) return;
    aiocbs->aio_offset = offset;
    if(aio_read(aiocbs) == -1)
        ERR("aio_read");
}

void WriteData(struct aiocb *aiocbs, off_t offset)
{
    if(!work) return;
    aiocbs->aio_offset = offset;
    if(aio_write(aiocbs) == -1)
        ERR("aio_write");
}

void SyncData(struct aiocb *aiocbs)
{
    if(!work) return;
    Suspend(aiocbs);
    if(aio_fsync(O_SYNC, aiocbs) == -1)
        ERR("aio_fsync");
    Suspend(aiocbs);
}

void Cleanup(char ** buffer, int fd)
{
    if(!work) 
        if(aio_cancel(fd, NULL) == -1)
            ERR("aio_cancel");
    for(int i=0; i<BLOCKS; i++)
        free(buffer[i]);
    if(TEMP_FAILURE_RETRY(fsync(fd)) == -1)
        ERR("fsync");
    
}

void ReadArguments(int argc, char** argv, int* pS, char** pN)
{
    if(argc != 3) usage(argv[0]);
    *pN = argv[1];
    *pS = atoi(argv[2]);
    if(*pS < 1 || *pS  > 10) usage(argv[0]);
    *pS *= KB;
}

void SetUp(int s, char *n, char **buffer, struct aiocb *aiocbs)
{
    if(chdir(n))    ERR("chdir");
    work = 1;
    for(int i=0; i<BLOCKS; i++)
        if((buffer[i] = (char*) calloc(s, sizeof(char))) == NULL)
            ERR("calloc");
    FillAIOStructs(aiocbs, buffer, s);
}

void Work(int s, char **buffer, struct aiocb *aiocbs)
{
    DIR *dirp;
	struct dirent *dp;
    struct stat attrib;
    size_t size;
    int fd;
    int curpos=0;
    threadArgs_t threadArgs[BLOCKS];
    for(int i=0;i<BLOCKS; i++)
    {
        threadArgs[i].pAiocb = &aiocbs[i];
        threadArgs[i].s = s;
    }

    if((dirp = opendir(".")) == NULL) ERR("opendir");
    errno = 0;
	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
            lstat(dp->d_name, &attrib);                
            if((size =  attrib.st_size) <= s )
            {
                if(curpos == BLOCKS) 
                {
                    curpos = 0;
                    JoinThreads(threadArgs, BLOCKS); // Close!!
                }       
                fprintf(stdout, "%s\t\tSize: %ld\n",dp->d_name, size); 
                if((fd = TEMP_FAILURE_RETRY(open(dp->d_name,O_RDWR))) < 0) ERR("open");
                aiocbs[curpos].aio_fildes = fd; 
                ReadData(&aiocbs[curpos],0);   
                //SyncData(&aiocbs[curpos]);
                if( pthread_create(&(threadArgs[curpos].tid), NULL, threadWork, &(threadArgs[curpos])) != 0) ERR("phtread_create");   
                curpos++;
            }
                
		}
	} while (dp != NULL);
    JoinThreads(threadArgs, curpos);
	if (errno != 0) ERR("readdir");    
	if(closedir(dirp)) ERR("closedir");    
}

void* threadWork(void* voidArgs)
{
    threadArgs_t* args = voidArgs;
    int readingCaret = 0, c, writingCaret = 0;
    volatile char* buffor = args->pAiocb->aio_buf;
    Suspend(args->pAiocb); 

    for(; readingCaret < args->s && buffor[readingCaret] != '\0'; readingCaret++)
    {
        c= buffor[readingCaret];
        if(c != 'a' && c != 'e' && c != 'i' && c != 'o' && c != 'u' && c != 'y')
        {     
            buffor[writingCaret] = c;
            writingCaret++;    
        }                          
    }

    ftruncate(args->pAiocb->aio_fildes,0);
    args->pAiocb->aio_nbytes = writingCaret;
    WriteData(args->pAiocb,0);
    Suspend(args->pAiocb); 
    args->pAiocb->aio_nbytes = args->s;
    
    memset((void*)buffor, 0, args->s);
    if(close(args->pAiocb->aio_fildes)) ERR("close");  
    return NULL;
}

void JoinThreads(threadArgs_t* args, int n)
{
    for(int i=0; i< n; i++)   
        if( pthread_join(args[i].tid, NULL) != 0) ERR("pthread_join");
    
}

int main(int argc, char** argv)
{
    int s; // Rozmiar bloków
    char *n; // Nazwa pliku
    char *buffer[BLOCKS];
    struct aiocb aiocbs[BLOCKS]; // Z jakiegoś powodu w tutorialu jest 4

    ReadArguments(argc, argv, &s, &n);
    SetUp(s, n, buffer, aiocbs);
    Work(s, buffer, aiocbs);
    return EXIT_SUCCESS;
}