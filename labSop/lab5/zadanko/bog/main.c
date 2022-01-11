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

void error(char *msg){
	perror(msg);
	exit(EXIT_FAILURE);
}

typedef struct StringVector {
    char** array;
    int size;
} StringVector;

void initStringVector(StringVector* vector) {
    vector->array = NULL;
    vector->size = 0;
}

void pushBack(StringVector* vector, char* value) {
    vector->array = (char**) realloc(vector->array, (vector->size + 1) * sizeof(char*));
    if(vector->array == NULL)
        error("realloc");
    vector->array[vector->size] = (char*) malloc((strlen(value) + 1) * sizeof(char));
    if(vector->array[vector->size] == NULL)
        error("malloc");
    strcpy(vector->array[vector->size], value);
    vector->size++;
}

void freeStringVector(StringVector* vector) {
    for(int i = 0; i < vector->size; i++) {
        free(vector->array[i]);
    }
    free(vector->array);
}

StringVector getFilesToRead(int s) {
    StringVector result;
    initStringVector(&result);
    DIR* dirp;
    struct dirent *dp;
	struct stat filestat;
    if(NULL == (dirp = opendir(".")))
        error("opendir");
    do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			if(lstat(dp->d_name, &filestat)) error("lstat");
			if(filestat.st_size < s && S_ISREG(filestat.st_mode)) {
                printf("%s %ld\n", dp->d_name, filestat.st_size);
                pushBack(&result, dp->d_name);
            }
		}
	} while (dp != NULL);
	if (errno != 0) error("readdir");
	if(closedir(dirp)) error("closedir");
    return result;
}

void fillaiocb(struct aiocb* aiocbObj, int file, char* buff, int buffSize) {
    memset(aiocbObj, 0, sizeof(struct aiocb));
    aiocbObj->aio_fildes = file;
    aiocbObj->aio_offset = 0;
    aiocbObj->aio_nbytes = buffSize;
    aiocbObj->aio_buf = (void*) buff;
    aiocbObj->aio_sigevent.sigev_notify = SIGEV_NONE;
}

void doSomethingWithBuffer(char* buff, int buffSize) {
    for(int i = 0; i < buffSize; i++)
        buff[i] = 'a';
}

void suspend(struct aiocb* aiocbObj) {
    aio_fsync(O_SYNC, aiocbObj);
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

int main(int argc, char** argv) {
    if(argc != 3)
        error("argc");
    char* directoryPath = argv[1];
    int s = atoi(argv[2]);
    s *= 1024;
    if(chdir(directoryPath) == -1)
        error("chdir");
    struct aiocb aiocbs[3];
    StringVector filesToRead = getFilesToRead(s);
    char* buffs[3];
    for(int i = 0; i < 3; i++) {
        buffs[i] = (char*) malloc(s * sizeof(char));
        if(buffs[i] == NULL)
            error("malloc");
    }
    int fileIter = 0;
    while(fileIter < filesToRead.size) {
        int f = open(filesToRead.array[fileIter], O_RDWR);
        fillaiocb(&aiocbs[0], f, buffs[0], getFileSize(filesToRead.array[fileIter]));
        aio_read(&aiocbs[0]);
        if(fileIter + 1 < filesToRead.size) {
            f = open(filesToRead.array[fileIter + 1], O_RDWR);
            fillaiocb(&aiocbs[1], f, buffs[1], getFileSize(filesToRead.array[fileIter + 1]));
            aio_read(&aiocbs[1]);
        }
        if(fileIter + 2 < filesToRead.size) {
            f = open(filesToRead.array[fileIter + 2], O_RDWR);
            fillaiocb(&aiocbs[2], f, buffs[2], getFileSize(filesToRead.array[fileIter + 2]));
            aio_read(&aiocbs[2]);
        }
        suspend(&aiocbs[0]);
        if(fileIter + 1 < filesToRead.size) suspend(&aiocbs[1]);
        if(fileIter + 2 < filesToRead.size) suspend(&aiocbs[2]);
        doSomethingWithBuffer(buffs[0], getFileSize(filesToRead.array[fileIter]));
        aio_write(&aiocbs[0]);
        if(fileIter + 1 < filesToRead.size) {
            doSomethingWithBuffer(buffs[1], getFileSize(filesToRead.array[fileIter + 1]));
            aio_write(&aiocbs[1]);
        }
        if(fileIter + 2 < filesToRead.size) {
            doSomethingWithBuffer(buffs[2], getFileSize(filesToRead.array[fileIter + 2]));
            aio_write(&aiocbs[2]);
        }
        for(int i = 0; i < getFileSize(filesToRead.array[fileIter]); i++) {
            printf("%c", buffs[0][i]);
        }
        printf("\n");
        if(fileIter + 1 < filesToRead.size) {
            for(int i = 0; i < getFileSize(filesToRead.array[fileIter + 1]); i++)
                printf("%c", buffs[1][i]);
            printf("\n");
        }
        if(fileIter + 2 < filesToRead.size) {
            for(int i = 0; i < getFileSize(filesToRead.array[fileIter + 2]); i++)
                printf("%c", buffs[2][i]);
            printf("\n");
        }
        suspend(&aiocbs[0]);
        if(fileIter + 1 < filesToRead.size) suspend(&aiocbs[1]);
        if(fileIter + 2 < filesToRead.size) suspend(&aiocbs[2]);
        fileIter += 3;
    }
    freeStringVector(&filesToRead);
    for(int i = 0; i < 3; i++)
        free(buffs[i]);
    return 0;
}
