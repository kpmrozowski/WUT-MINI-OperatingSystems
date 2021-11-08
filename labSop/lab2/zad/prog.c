#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#define MAXFD 20
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))


void etap1 (){
	DIR* dirp;
	char* name = NULL;
	struct stat filestat;
	struct dirent *dp;
	int size=0;
	FILE* s1;
	if (name != NULL)
		if((s1=fopen(name,"w+"))==NULL)ERR("fopen");
	if(NULL == (dirp = opendir("."))) ERR("opendir");
	do {
		errno = 0;
		if((dp = readdir(dirp)) != NULL) {
			if (lstat(dp->d_name, &filestat)) ERR("lstat");
			size+=filestat.st_size;
			if (name == NULL)
				printf("%s\n", dp->d_name);
			else
				fprintf(s1,"%s\n", dp->d_name);
		}
	} while (dp != NULL);
	if (errno != 0) ERR("readdir");
	if(closedir(dirp)) ERR("closedir");
	if (name == NULL)
		printf("ZAJETOSC: %d\n", size);
	else
	{
		fprintf(s1,"%s\n", dp->d_name);
		if(fclose(s1))ERR("fclose");
	}
}

		 
int main(int argc, char** argv) {

		char c;
        while ((c = getopt (argc, argv, "p:o:")) != -1)
                switch (c)
                {
                        case 'p':
                                chdir(optarg);
                                etap1();
                                break;
                        case 'o':
                                break;
                        case '?':
                        default:
                                ;
                }
        
	return EXIT_SUCCESS;
}
