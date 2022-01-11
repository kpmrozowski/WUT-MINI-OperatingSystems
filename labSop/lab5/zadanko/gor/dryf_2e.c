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
#include <dirent.h>
#include <stddef.h>
#include <pthread.h>
#include <stdarg.h>
#define MAX_NAME 50
#define BUFORY 3
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

typedef struct argsEstimation {
	pthread_t tid;
	char *tekst;
	struct aiocb cb;
} argsEstimation_t;			 

void scan_dir(int granica);
void error(char *msg);
void fillaiostructs(struct aiocb *, char **, int, int);
volatile sig_atomic_t work;
void suspend(struct aiocb *aiocbs);
void cleanup(char **buffers, int fd);
void readdata(struct aiocb *aiocbs, off_t offset);
void* praca_watku(void *voidPtr);

void* praca_watku(void *voidPtr) {
	argsEstimation_t *args = voidPtr;
	int i = 0;
	char c;
	while(args->tekst[i]!='\0'){
		c = args->tekst[i];
		if(c!='a' && c!='e' && c!='i' && c!='o'&& c!='u' && c!='y')
		{
			putchar(c);
		}
		i++;
	}
	suspend(&args->cb);
	printf("\n");
	sleep(1);
	return NULL;
}

void readdata(struct aiocb *aiocbs, off_t offset){
	if (!work) return;
	aiocbs->aio_offset = offset;
	if (aio_read(aiocbs) == -1)
		error("Cannot read");
}

void cleanup(char **buffers, int fd){
	int i;
	if (!work)
		if (aio_cancel(fd, NULL) == -1)
			error("Cannot cancel async. I/O operations");
	for (i = 0; i<BUFORY; i++)
		free(buffers[i]);
	if (TEMP_FAILURE_RETRY(fsync(fd)) == -1)
		error("Error running fsync");
}

void suspend(struct aiocb *aiocbs){
	struct aiocb *aiolist[1];
	aiolist[0] = aiocbs;
	if (!work) return;
	while (aio_suspend((const struct aiocb *const *) aiolist, 1, NULL) == -1){
		if (!work) return;
		if (errno == EINTR) continue;
		error("Suspend error");
	}
	if (aio_error(aiocbs) != 0)
		error("Suspend error");
	if (aio_return(aiocbs) == -1)
		error("Return error");
}

void fillaiostructs(struct aiocb *aiocbs, char **buffer, int fd, int blocksize){
	int i;
	for (i = 0; i<BUFORY; i++){
		memset(&aiocbs[i], 0, sizeof(struct aiocb));
		aiocbs[i].aio_fildes = fd;
		aiocbs[i].aio_offset = 0;
		aiocbs[i].aio_nbytes = blocksize;
		aiocbs[i].aio_buf = (void *) buffer[i];
		aiocbs[i].aio_sigevent.sigev_notify = SIGEV_NONE;
	}}

void error(char *msg){
	perror(msg);
	exit(EXIT_FAILURE);
}

void scan_dir (int granica){
	DIR *dirp; //to jest katalog
	struct dirent *dp;//tu wczytujemy kolejne rzeczy z katalogu
	struct stat filestat;//tu zapisujemy dane z danej rzeczy
	if(NULL == (dirp = opendir("."))) ERR("opendir");
	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			if (lstat(dp->d_name, &filestat)) ERR("lstat");
            if(filestat.st_size<granica){
                printf("Nazwa pliku: %s, rozmiar pliku: %ld\n",dp->d_name, filestat.st_size);
            }
		}
	} while (dp != NULL);
	if (errno != 0) ERR("readdir");
	if(closedir(dirp)) ERR("closedir");
}

int main(int argc, char ** argv)//nazwa, rozmiar
{   
    int limit = atoi(argv[2]);
    work = 1;
    char *buffer[BUFORY];
    int i;
    // char *pierwszy;
    for (i = 0; i<BUFORY; i++)
			if ((buffer[i] = (char *) calloc (limit, sizeof(char))) == NULL)
				error("Cannot allocate memory");
    chdir(argv[1]);
    scan_dir(limit);
    struct aiocb aiocbs[4];
    int fd;
    DIR *dirp;
    if (NULL == (dirp = opendir("."))) ERR("opendir");
    struct dirent *dp;
    struct stat filestat;
    if ((dp = readdir(dirp)) != NULL) {
			if (lstat(dp->d_name, &filestat)) ERR("lstat");}
    if ((fd = TEMP_FAILURE_RETRY(open(dp->d_name, O_RDWR))) == -1)
		error("Cannot open file");
    fillaiostructs(aiocbs,buffer,fd,limit);
    readdata(&aiocbs[0], 0);
	suspend(&aiocbs[0]);
	//watek-start
	argsEstimation_t* estimations = (argsEstimation_t*) malloc(sizeof(argsEstimation_t));
	if (estimations == NULL) ERR("Malloc error for estimation arguments!");
	estimations[0].tekst = buffer[0];
	estimations[0].cb = aiocbs[0];
	int err = pthread_create(&(estimations[0].tid), NULL, praca_watku, &estimations[0]);
		if (err != 0) ERR("Couldn't create thread");
	err = pthread_join(estimations[0].tid, NULL);
		if (err != 0) ERR("Can't join with a thread");

	//watek-koniec
    cleanup(buffer, fd);
    return EXIT_SUCCESS;
}