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
#include <stdlib.h>
#include <stdbool.h>

#define aio_OPEN(_fname )\
CreateFile(_fname,                       \
           GENERIC_READ | GENERIC_WRITE, \
           FILE_SHARE_READ,              \
           NULL,                         \
           OPEN_ALWAYS,                  \
           FILE_ATTRIBUTE_NORMAL,        \
           NULL)

void error(char *msg){
	perror(msg);
	exit(EXIT_FAILURE);
}

void fillaiocb(struct aiocb* aiocbObj, int file, char* buff, int buffSize, int offset) {
    memset(aiocbObj, 0, sizeof(struct aiocb));
    aiocbObj->aio_fildes = file;
    aiocbObj->aio_offset = offset;
    aiocbObj->aio_nbytes = buffSize;
    aiocbObj->aio_buf = (void*) buff;
    aiocbObj->aio_sigevent.sigev_notify = SIGEV_NONE;
}

void doSomethingWithBuffer(char* buff, int buffSize) {
    // for(int j = 0; j < buffSize; j++)
    //     buff[j] += 0;
}

void suspend(struct aiocb* aiocbObj) {
    struct aiocb *aiolist[1];
	aiolist[0] = aiocbObj;
	while(aio_suspend((const struct aiocb *const *) aiolist, 1, NULL) == -1) {
	    if(errno == EINTR) continue;
		error("Suspend error");
	}
    if(aio_error(aiocbObj) != 0)
		error("Suspend error");
	if(aio_return(aiocbObj) == -1)
		error("Return error");
}

int getFileSize(char* fileName) {
    struct stat filestat;
    if(lstat(fileName, &filestat)) error("lstat");
    return filestat.st_size;
}

off_t getfilelength(int fd){
        struct stat buf;
        if (fstat(fd, &buf) == -1)
                error("Cannot fstat file");
        return buf.st_size;
}

int main(int argc, char** argv) {
    printf("start\n");
    if(argc != 3)
        error("argc");
    char* filename = argv[1];
    int s = atoi(argv[2]);
    int fd;
    if ((fd = TEMP_FAILURE_RETRY(open(filename, O_RDWR))) == -1)
        error("Cannot open file");
    
    int filesize = getfilelength(fd);
    printf("filesize=%d\n", filesize);
    int aiocbs_count = filesize / s + 1;
    printf("aiocbs_count=%d\n", aiocbs_count);
    bool* not_readed = (bool*) malloc(aiocbs_count * sizeof(bool));
    memset(not_readed, 1, aiocbs_count);
    int left_to_read = aiocbs_count;
    
    char* buffs[aiocbs_count];
    for(int i = 0; i < aiocbs_count; i++) {
        buffs[i] = (char*) malloc(s * sizeof(char));
        if(buffs[i] == NULL)
            error("malloc");
    }
    char* buff_write;
    buff_write = (char*) malloc(s * sizeof(char));
    if(buff_write == NULL)
        error("malloc_write");
    int fds[aiocbs_count];
    for (int i = 0; i < aiocbs_count; ++i) {
        char* name = i + '0';
        if ((fds[i] = TEMP_FAILURE_RETRY(open(name, O_DSYNC))) == -1)
            error("Cannot open file to write");
    }
    
    struct aiocb aiocbs[aiocbs_count];
    struct aiocb aiocbs_write[1];
    // fillaiocb(&aiocbs[i], fd, buffs[i], s, i * s);

    for (int i = 0; i < aiocbs_count; ++i) {
        fillaiocb(&aiocbs[i], fd, buffs[i], s, i * s);
        aio_read(&aiocbs[i]);
        suspend(&aiocbs[i]);
        doSomethingWithBuffer(buffs[i], s);
        aio_write(&aiocbs[i]);
        for(int j = 0; j < s; j++) {
            printf("%c", buffs[i][j]);
        }
        suspend(&aiocbs[i]);
    }
    printf("ok\n");
    for(int i = 0; i < aiocbs_count; i++)
        free(buffs[i]);
    free(buff_write);
    free(not_readed);
    return 0;
}
