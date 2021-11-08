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


void usage(char* pname){
	fprintf(stderr,"USAGE:%s ([-t x] -n Name) ... \n",pname);
	exit(EXIT_FAILURE);
}
	
	
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

void make_file(char *name, ssize_t size, mode_t perms, int percent){
        FILE* s1;
        int i;
        umask(~perms&0777);
        if((s1=fopen(name,"w+"))==NULL)ERR("fopen");
        for(i=0;i<(size*percent)/100;i++){
                if(fseek(s1,rand()%size,SEEK_SET)) ERR("fseek");
                fprintf(s1,"%c",'A'+(i%('Z'-'A'+1)));
        }
        if(fclose(s1))ERR("fclose");
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

		char c;
		char* name;
		mode_t perms=-1;
        ssize_t size=-1;
        while ((c = getopt (argc, argv, "p:n:s:")) != -1)
                switch (c)
                {
                        case 'p':
                                perms=strtol(optarg, (char **)NULL, 8);
                                break;
                        case 's':
                                size=strtol(optarg, (char **)NULL, 10);
                                break;
                        case 'n':
                                name=optarg;
                                break;
                        case '?':
                        default:
                                usage(argv[0]);
                }
        if((NULL==name)||(-1==perms)||(-1==size)) usage(argv[0]);
        if(unlink(name)&&errno!=ENOENT)ERR("unlink");
        srand(time(NULL));
        make_file(name,size,perms,10);
	return EXIT_SUCCESS;
}
