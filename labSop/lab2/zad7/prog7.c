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

	extern char **environ;
	int index = 0;
	while (environ[index])
		printf("%s\n", environ[index++]);
	return EXIT_SUCCESS;
}
