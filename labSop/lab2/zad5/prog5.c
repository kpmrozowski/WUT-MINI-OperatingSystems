#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void usage(char* pname){
	fprintf(stderr,"USAGE:%s name times>0\n", pname);
	exit(EXIT_FAILURE);
}

int main (int argc, char** argv)
{
	if(argc!=3) usage(argv[0]);
	int i,j=atoi(argv[2]);
	for(i=0;i<j;i++)
		printf("Hello %s\n",argv[1]);
	return EXIT_SUCCESS;
}
