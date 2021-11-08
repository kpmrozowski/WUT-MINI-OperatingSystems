#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ftw.h>
#define MAXFD 20
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))
#define MAX_PATH 60

void scan_dir (){
	DIR *dirp;
	struct dirent *dp;
	struct stat filestat;
	int dirs=0,files=0,links=0,other=0;
	if(NULL == (dirp = opendir("."))) ERR("opendir");
	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			if (lstat(dp->d_name, &filestat)) ERR("lstat");
			if (S_ISDIR(filestat.st_mode)) dirs++;
			else if (S_ISREG(filestat.st_mode)) files++;
			else if (S_ISLNK(filestat.st_mode)) links++;
			else other++;
		}
	} while (dp != NULL);
	if (errno != 0) ERR("readdir");
	if(closedir(dirp)) ERR("closedir");
	printf("Files: %d, Dirs: %d, Links: %d, Other: %d\n",files,dirs,links,other);
}

int walk(const char *name, const struct stat *s, int type, struct FTW *f){
        if(FTW_D==type){
                printf("%s:\n",name);
                chdir(name+f->base);
                scan_dir();
        }
        return 0;
}


		 
int main(int argc, char** argv) {

	int i;
        for(i=1;i<argc;i++){
                nftw(argv[i],walk,MAXFD,FTW_CHDIR);
        }
	return EXIT_SUCCESS;
}
