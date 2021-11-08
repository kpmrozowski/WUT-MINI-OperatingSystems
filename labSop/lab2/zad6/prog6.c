#include <stdio.h>
#include <stdlib.h>
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))
#define MAX_LINE 20

void usage(char* pname){
	fprintf(stderr,"USAGE:%s ([-t x] -n Name) ... \n",pname);
	exit(EXIT_FAILURE);
}		     
		 
int main(int argc, char** argv) {

	int x = 1;
	while ((c = getopt (argc, argv, "t:n:")) != -1)
		switch (c){
			case 't':
				x=atoi(optarg);
				break;
			case 'n':
				for(i=0;i<x;i++)
					printf("Hello %s\n",optarg);
				break;
			case '?':
			default: usage(argv[0]);
		}
	if(argc>optind) usage(argv[0]);
	return EXIT_SUCCESS;
}
