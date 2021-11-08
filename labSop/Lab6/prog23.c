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

struct arg
{
	char *buffer;
};

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

void *workfile (void *argi)
{
	struct arg *tab = argi;
	printf("%s", tab->buffer);
}

int main(int argc, char *argv[])
{
	char *dirname, *filename, *buffer[2];
	int s, fd, fsize, i, fcount, max, err;
	struct aiocb aiocbs[2];
	DIR* dir;
	struct dirent* dp;
	int *files;
	pthread_t tid;
	struct arg tab;
	max = 10;
	fcount = 0;
		
		
	if (argc != 3)
		usage(argv[0]);
	dirname = argv[1];
	s = atoi(argv[2]);
	
	if ((dir = opendir(dirname)) == NULL)
		error("Cannot open file");
		
	if((filename = (char *)calloc(20,sizeof(char))) == NULL)
		error("calloc error");
		
	if((files = (int *)calloc(max, sizeof(int))) == NULL)
		error("calloc error");
		
	
	
	
	do
	{
		if ((dp = readdir(dir)) != NULL)
		{
			filename = strcpy(filename, dirname);
			filename = strcat(filename, "/");
			filename = strcat(filename, dp->d_name);
			if(!((strcmp(dp->d_name,".")==0)||(strcmp(dp->d_name , "..") == 0)))
			{
				if ((fd = TEMP_FAILURE_RETRY(open(filename, O_RDWR))) == -1)
					error("Cannot open file");
				fsize = getfilelength(fd);
				if (fsize < s*1000)
				{
					if (fcount++>max)
					{
						max*=2;
						if ((files = realloc(files,max*sizeof(int))) == NULL)
							error("realloc error");
					}
					files[fcount-1] = fd;
					//printf("%s, size: %d\n",dp->d_name, fsize);
				}
			}
		}
	} while (dp != NULL);
	
	for(i = 0; i<2; i++)
	{
		if ((buffer[i] = (char *)calloc(s*1000,sizeof(char))) == NULL)
			error("calloc error");
		fillaiostruct(&aiocbs[i], buffer[i], files[i], s*1000, 0);
	}
	
	read(files[0],buffer[0],s*1000);
	//printf("%s", buffer[0]);
	tab.buffer = buffer[0];
	err = pthread_create(&tid, NULL, workfile,&tab);
	if (err != 0) error("create thread error");
	sleep(1);
	 
	
	
	closedir(dir);
	for (i = 0; i<2; i++)
		free(buffer[i]);
	free(files);
	free(filename);
		
	
	return EXIT_SUCCESS;
}
