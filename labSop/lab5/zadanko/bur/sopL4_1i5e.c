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
#include <pthread.h>
#include <time.h>
#define BLOCKS 3
#define MAX_PATH 40
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

typedef struct argss {
        pthread_t tid;
        struct aiocb aio;
        char *buff;
} argss;
void scan_dir ();
void error(char *msg){
	perror(msg);
	exit(EXIT_FAILURE);
}
void usage(char *progname){
	fprintf(stderr, "%s - Bad arguments\n", progname);
	exit(EXIT_FAILURE);
}

void fillaiostructs(struct aiocb *aiocbs, char **buffer, int fd, int blocksize){
	int i;
	for (i = 0; i<BLOCKS; i++){
		memset(&aiocbs[i], 0, sizeof(struct aiocb));
		aiocbs[i].aio_fildes = fd;
		aiocbs[i].aio_offset = 0;
		aiocbs[i].aio_nbytes = blocksize;
		aiocbs[i].aio_buf = (void *) buffer[i];
		aiocbs[i].aio_sigevent.sigev_notify = SIGEV_NONE;
	}
}

void writedata(struct aiocb *aiocbs, off_t offset){
	aiocbs->aio_offset = offset;
	if (aio_write(aiocbs) == -1)
		error("Cannot write");
}

void readdata(struct aiocb *aiocbs, off_t offset){
	aiocbs->aio_offset = offset;
	if (aio_read(aiocbs) == -1)
		error("Cannot read");
}

void* work_fun(void* voidArgs)
{
    argss* args = voidArgs;
    char tmp;
    char *b=malloc(sizeof(args->buff));
    int j=0;
    for(int i=0;i<args->aio.aio_nbytes;i++)
    {
        tmp=args->buff[i];
        if(tmp!='a' && tmp!='e' && tmp!='i' && tmp!='o' && tmp!='u' && tmp!='y')
        {
            b[j]=tmp;
            j++;
        }
    }
    char *c=malloc(sizeof(char)*(j+1));
    strncpy(c,b,j);
    free(b);
    c[j]='\0';
    printf("%s\n",c);
    //writedata(&args->aio,j);
    return NULL;
}

void scan_d(char *dirname, int s)
{
    char path[MAX_PATH];
	if(getcwd(path, MAX_PATH)==NULL) ERR("getcwd");
	if(chdir(dirname))ERR("chdir");
	printf("%s:\n",dirname);
	scan_dir(s);
	if(chdir(path))ERR("chdir");
}

void scan_dir (int s){
    char *buffer[BLOCKS];
    struct aiocb aiocbs[4];
    for (int i = 0; i<BLOCKS; i++)
	    if ((buffer[i] = (char *) calloc (s, sizeof(char))) == NULL)
		    error("Cannot allocate memory");
	DIR *dirp;
	struct dirent *dp;
	struct stat filestat;
    int sz, fd;
	if (NULL == (dirp = opendir("."))) ERR("opendir");
	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			if (lstat(dp->d_name, &filestat)) ERR("lstat");
			if (S_ISREG(filestat.st_mode))
            {
                sz = filestat.st_size;
                if(sz<=10*1024)
                {
                    printf("%s - size: %d\n",dp->d_name, sz);
                    if ((fd = TEMP_FAILURE_RETRY(open(dp->d_name, O_RDWR))) == -1)
                    {
                        error("Cannot open file");
                    }
                    fillaiostructs(aiocbs, buffer, fd, s);
                    readdata(&aiocbs[1], s);
                    argss args;
                    args.buff=buffer[1];
                    args.aio=aiocbs[1];
                    int err = pthread_create(&args.tid, NULL, work_fun, &args);
                    if (err != 0) ERR("Couldn't create thread");
                    struct timespec req;
                    req.tv_sec = 1;
                    req.tv_nsec = 0;
                    if(nanosleep(&req,&req)) ERR("nanosleep");
                    if(pthread_join(args.tid, NULL)) 
                        ERR("Failed to join with a student thread!");
                    return;
                }
            }

		}
	} while (dp != NULL);

	if (errno != 0) ERR("readdir");
	if(closedir(dirp)) ERR("closedir");
}



int main(int argc, char *argv[]){
	char *n;
	if (argc != 3)
		usage(argv[0]);
	n = argv[1];
	int s = atoi(argv[2]);
	if (s < 1 || s > 10)
		return EXIT_SUCCESS;

    
    scan_d(n,s);

	/*srand(time(NULL));
	processblocks(aiocbs, buffer, n, s, k);
	cleanup(buffer, fd);
	if (TEMP_FAILURE_RETRY(close(fd)) == -1)
		error("Cannot close file");*/

	return EXIT_SUCCESS;
}