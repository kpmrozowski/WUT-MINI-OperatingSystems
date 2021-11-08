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

struct arg
{
	char* buffer;
	int offset;
	int sig;
}

void error(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void fillaiostruct(struct aiocb *aiocbs, char *buffer, int fd, int blocksize, int offset)
{
	memset(aiocbs, 0, sizeof(struct aiocb));
	aiocbs->aio_fildes = fd;
	aiocbs->aio_offset = offset;
	aiocbs->aio_nbytes = blocksize;
	aiocbs->aio_buf = (void *) buffer;
	aiocbs->aio_sigevent.sigev_notify = SIGEV_NONE;

}


void usage(char *progname)
{
	fprintf(stderr, "%s workfile blocksize\n", progname);
	fprintf(stderr, "workfile - path to the file to work on\n");
	fprintf(stderr, "n - number of blocks\n");
	fprintf(stderr, "k - number of iterations\n");
	exit(EXIT_FAILURE);
}

off_t getfilelength(int fd)
{
	struct stat buf;
	if (fstat(fd, &buf) == -1)
		error("Cannot fstat file");
	return buf.st_size;
}

void* filework(void* args)
{
	struct arg tab = args;
	printf("offset: %d", tab.offset);
}

int main(int argc, char *argv[])
{
	char* filename, buffer[4];
	int fd, k, fsize, buffersize, rest, i;
	struct aiocb aiocbs[2];
		
	if (argc != 3)
		usage(argv[0]);
	filename = argv[1];
	k = atoi(argv[2]);
	
	if ((fd = TEMP_FAILURE_RETRY(open(filename, O_RDWR))) == -1)
		error("Cannot open file");

		
	fsize = getfilelength(fd)-1;
	buffersize = fsize / k;
	rest = buffersize * k;
	if(buffersize == 0) error("bufor");
	if(k>=2 && buffersize>0)
	{
		for(i=0;i<4;i++)
		{
			if ((buffer[i] = (char *)calloc(buffersize, sizeof(char))) == NULL)
				error("buffer calloc error");
		}
		for(i=0;i<2;i++)
		{
			fillaiostruct(&aiocbs[i], buffer[i],fd,buffersize,(i%2)*buffersize);
			printf("offset%d: %d\n",i,(i%2)*buffersize);
		}
		for (i=0;i<k/2;i++)
		{
			
		}
	}
	printf("koncowy offset: %d\n", (!(k%2)) ? rest : rest - buffersize);
	return EXIT_SUCCESS;
}
