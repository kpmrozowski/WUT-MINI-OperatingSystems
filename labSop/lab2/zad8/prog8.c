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

	char *env = getenv("TIMES");
	int x,i;
	if(env) x = atoi(env);
	else x = 1;
	char name[MAX_LINE+2];
	while(fgets(name,MAX_LINE+2,stdin)!=NULL)
		for(i=0;i<x;i++)
			printf("Hello %s",name);
	if(putenv("RESULT=Done")!=0) {
		fprintf(stderr,"putenv failed");
		return EXIT_FAILURE;
	}
        printf("%s\n",getenv("RESULT"));
        if(system("env|grep RESULT")!=0)
                return EXIT_FAILURE;
    setenv("RESULT", "DONE",0);
	return EXIT_SUCCESS;
}
