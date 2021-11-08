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
#include <time.h>
#include <pthread.h>

// MACROS
#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))
// CONSTANTS
#define MAXLINE 4096
#define BUFFER_AMOUNT 2
#define AIOCBS_AMOUNT 2

// TYPEDEFS
typedef struct timespec timespec_t;
typedef struct aiocb aiocb_t;
typedef struct stat finfo_t;
typedef unsigned int UINT;
typedef sigset_t mask_t;
typedef pthread_mutex_t mutex_t;


// STRUCTURES
typedef struct args {
	int fd;
	int blockAmount;
	int blockSize;
	int remaining;
	int fileSize;
	char* fileName;
} args_t;

// FUNCTIONS
void msleep_safe(UINT milisec) {
    time_t sec= (int)(milisec/1000);
    milisec = milisec - (sec*1000);
    timespec_t req= {0,0};
    req.tv_sec = sec;
    req.tv_nsec = milisec * 1000000L;
    int err;
    while ((err=nanosleep(&req,&req))!=0);
}
/*
 * Get file length
 */ 
off_t getfilelength(int fd)
{
	struct stat buf;
	if (fstat(fd, &buf) == -1)
		ERR("fstat");
	return buf.st_size;
}


ssize_t bulk_read(int fd, char *buf, size_t count){
        ssize_t c;
        ssize_t len=0;
        do{
                c=TEMP_FAILURE_RETRY(read(fd,buf,count));
                if(c<0) return c;
                if(c==0) return len; //EOF
                buf+=c;
                len+=c;
                count-=c;
        }while(count>0);
        return len ;
}
ssize_t bulk_write(int fd, char *buf, size_t count){
        ssize_t c;
        ssize_t len=0;
        do{
                c=TEMP_FAILURE_RETRY(write(fd,buf,count));
                if(c<0) return c;
                buf+=c;
                len+=c;
                count-=c;
        }while(count>0);
        return len ;
}   
/*
 * Print help how to use the program
 */ 
void usage(char* name){
	printf("%s [n] [k]\n",name);
	printf("n - file path\n");
	printf("k - block amount >2\n");
	exit(EXIT_FAILURE);
}
/*
 * Get arguments from the execution command
 */ 
void get_arguments(int argc,char** argv,args_t* arguments){
	if (argc<3) usage(argv[0]);
	arguments->fileName = argv[1];
	arguments->blockAmount = atoi(argv[2]);
	if (arguments->blockAmount<2) usage(argv[0]);
	return;
}
/*
 * Async write operation
 */ 
void awrite(aiocb_t *aiocb){
	if (aio_write(aiocb)) ERR("aio_write");
}
/*
 * Async read operation
 */ 
void aread(aiocb_t *aiocb){
	if (aio_read(aiocb)) ERR("aio_read");
}
/*
 * Open 
 */ 
void open_files(args_t* ar){
	printf("Opening files...\n");
	if((ar->fd=TEMP_FAILURE_RETRY(open(ar->fileName,O_RDWR)))<0) ERR("open");
	ar->fileSize= (int)getfilelength(ar->fd);
	ar->remaining = ar->fileSize;
	ar->blockSize = ar->remaining/ar->blockAmount;
	ar->remaining = ar->remaining-(ar->remaining)%((2*ar->blockSize));
	 
}
void close_files(args_t* ar){
	printf("Closing files...\n");
	if (fsync(ar->fd)) ERR("fsync");
	if(TEMP_FAILURE_RETRY(close(ar->fd)))ERR("close");
}

void prep_aiocbs(aiocb_t* aiocbs, char** buffers, int fd, int blockSize){
	printf("Preparing aio control blocks...\n");
	int i;
	for(i=0;i<AIOCBS_AMOUNT;i++){
		memset(&aiocbs[i],0,sizeof(aiocb_t));
		aiocbs[i].aio_fildes = fd;
		aiocbs[i].aio_offset = 0;
		aiocbs[i].aio_nbytes = blockSize;
		aiocbs[i].aio_buf = buffers[i];
	}
}

void processText(char* text,int length){
	for (int i=0;i<length;i++){
		char c = text[i];
		if (c>='A' && c<='Z'){
			text[i] = c-'A'+'a';
		}
		else if (c>='a' && c<='z'){
			text[i] = c-'a'+'A';
		}
	}
}

void allocate_memory(char** buffers,int size){
	printf("Allocating memory...\n");
	int i;
	for (i=0;i<BUFFER_AMOUNT;i++){
		buffers[i] = (char*)malloc(size);
		if (!buffers[i]) {
			for (i=i-1;i>=0;i--){
				free(buffers[i]);
			}
			ERR("malloc");
		}
	}
}

void free_memory(char** buffers){
	printf("Freeing memory...\n");
	int i;
	for (i=0;i<BUFFER_AMOUNT;i++){
		free(buffers[i]);
	}
}

void aio_completion_handler(sigval_t sigval){
	mask_t oldMask, newMask;
    sigemptyset(&newMask);
    sigaddset(&newMask, SIGUSR1);
    sigaddset(&newMask, SIGIO);
	if (pthread_sigmask(SIG_BLOCK, &newMask, &oldMask)) ERR("SIG_BLOCK error");
	
	msleep_safe(50);
	
	aiocb_t *aiocb;
	aiocb = (aiocb_t*)sigval.sival_ptr;
	
	if(aio_error(aiocb)) ERR("aio_completion_handler aio_error");
	int ret;
	if ((ret=aio_return(aiocb))==-1) ERR("aio_completion_handler aio_return");
	
	processText((char*)aiocb->aio_buf,(int)aiocb->aio_nbytes);
	
	aiocb->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	int offset = (int)aiocb->aio_offset;
	int blockSize = (int)aiocb->aio_nbytes;
	if ((offset/blockSize)%2)
	aiocb->aio_sigevent.sigev_signo = SIGIO;
	else 
	aiocb->aio_sigevent.sigev_signo = SIGUSR1;
	awrite(aiocb);
	return;
}

void set_aiocbs(aiocb_t* aiocbs, int offset){
	int i;
	for (i=0;i<AIOCBS_AMOUNT;i++){
		aiocbs[i].aio_offset = (offset+i)*(int)aiocbs[i].aio_nbytes;
		aiocbs[i].aio_sigevent.sigev_notify = SIGEV_THREAD;
		aiocbs[i].aio_sigevent.sigev_notify_function = aio_completion_handler;
		aiocbs[i].aio_sigevent.sigev_notify_attributes = NULL;
		aiocbs[i].aio_sigevent.sigev_value.sival_ptr = &aiocbs[i];
	}
}


void handle_rest(args_t *ar){
	int restLength = ar->fileSize-ar->remaining;
	char* buffer = (char*)malloc(restLength);
	if (lseek(ar->fd,ar->remaining,SEEK_SET)==-1) ERR("lseek");
	if(bulk_read(ar->fd,buffer,restLength)<0) ERR("read");
	processText(buffer,restLength);
	if (lseek(ar->fd,ar->remaining,SEEK_SET)==-1) ERR("lseek");
	if(bulk_write(ar->fd,buffer,restLength)<0)ERR("write");
	free(buffer);	
}

void wait_for_signals(mask_t mask){
		int signo;
		int receivedIO=0;
		int receivedUSR1=0;
		while (1){
			//printf("pre sig wait\n");
			if(sigwait(&mask, &signo)) ERR("sigwait failed.");
			switch (signo){
				case SIGIO:
					receivedIO=1;
					//printf("Received IO\n");
					break;
				case SIGUSR1:
					//printf("Received SIGUSR1\n");
					receivedUSR1=1;
					break;
				default:
					break;
			}
			if (receivedIO&&receivedUSR1) break;
		}
}

void run(args_t *ar,aiocb_t* aiocbs,char** buffers,mask_t mask){
	int remainingBlocks = ar->blockAmount;
	int index =0;
	while (remainingBlocks>=2){
		set_aiocbs(aiocbs,index);
		int i;
		for (i=0;i<AIOCBS_AMOUNT;i++){
			aread(&aiocbs[i]);
		}
		wait_for_signals(mask);
		for (i=0;i<AIOCBS_AMOUNT;i++){
			if(aio_error(&aiocbs[i])) ERR("suspend aio_error");
			int ret;
			if ((ret=aio_return(&aiocbs[i]))==-1) ERR("suspend aio_return");
		}
		remainingBlocks-=2;
		index+=2;
	}
	handle_rest(ar);
}

/*
 * Main function
 */ 
int main(int argc, char** argv){
	char* buffers[BUFFER_AMOUNT];
	aiocb_t aiocbs[AIOCBS_AMOUNT];
	args_t ar;
	mask_t oldMask, newMask;
    sigemptyset(&newMask);
    sigaddset(&newMask, SIGUSR1);
    sigaddset(&newMask, SIGIO);
    //if (sigprocmask(SIG_BLOCK,&newMask,&oldMask)) ERR("SIG_BLOCK ERROR");
	if (pthread_sigmask(SIG_BLOCK, &newMask, &oldMask)) ERR("SIG_BLOCK error");
	
	get_arguments(argc,argv,&ar);
	open_files(&ar);
	allocate_memory(buffers,ar.blockSize);
	printf("Block amount: %d\n",ar.blockAmount);
	printf("Block size: %d\n",ar.blockSize);
	printf("Remaining: %d\n",ar.remaining);
	prep_aiocbs(aiocbs,buffers,ar.fd,ar.blockSize);
	run(&ar,aiocbs,buffers,newMask);
	free_memory(buffers);
	close_files(&ar);
	
	if (pthread_sigmask(SIG_UNBLOCK, &newMask, &oldMask)) ERR("SIG_BLOCK error");
	
	return EXIT_SUCCESS;
}
