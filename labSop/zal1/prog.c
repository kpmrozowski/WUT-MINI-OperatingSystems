#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#define MAX_LINE 20

#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))
#define MAX_PATH 60

void usage(char* pname){
	fprintf(stderr,"USAGE:%s ([-t x] -n Name) ... \n",pname);
	exit(EXIT_FAILURE);
}		

void l_s (char* path){
    char buf[512];
	DIR *dirp;
	struct dirent *dp;
	struct stat filestat;
	// int dirs=0,files=0,links=0,other=0;
	if(NULL == (dirp = opendir("."))) ERR("opendir");
	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			sprintf(buf, "%s/%s", path, dp->d_name);
			stat(buf, &filestat);
			if (strncmp(".",dp->d_name,1) != 0 && strncmp("..",dp->d_name,2) != 0)
				printf("%s\n", dp->d_name);
		}
	} while (dp != NULL);
	if (errno != 0) ERR("readdir");
	if(closedir(dirp)) ERR("closedir");
}

int main(int argc, char** argv) {

	char *v = getenv("VERBOSE");
	char *he = getenv("HANDLE_ERROR");
	int verbose=0, handle_error=0;
	if(v) verbose = atoi(v);
	if(he) handle_error = atoi(he);
    int c;
    char* path = ".";
    char* output = ".";
	while ((c = getopt (argc, argv, "p:o:")) != -1){
		switch (c){
			case 'p':
				path = optarg;
                if (verbose)
                    printf("%s\n", path);
                if(chdir(path))ERR("chdir");
                break;
			case 'o':
				output = optarg;
                if (handle_error)
                    printf("%s\n", output);
				break;
			case '?':
			default: usage(argv[0]);
		}
	}
	char cmd[MAX_LINE+2];
	while (fgets(cmd,MAX_LINE+2,stdin)!=NULL){
		int len = strlen(cmd);
		if( cmd[len-1] == '\n' )
			cmd[len-1] = 0;
		if (strncmp("ls",cmd,2) == 0){
			// printf("%s:\n",path);
			l_s(path);
		}
		if (strncmp("rm",cmd,2) == 0){
			if (remove(cmd+3) != 0)
				return EXIT_FAILURE;
			if (verbose) printf("file removed\n");
		}
		if (strncmp("exit",cmd,4) == 0){
			return EXIT_SUCCESS;
		}
	}
	if(argc>optind) usage(argv[0]);
	return EXIT_SUCCESS;
}
