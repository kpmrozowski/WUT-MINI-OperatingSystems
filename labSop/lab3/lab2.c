#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
                                     exit(EXIT_FAILURE))


volatile sig_atomic_t last_signal = 0;

void sethandler( void (*f)(int), int sigNo) {
        struct sigaction act;
        memset(&act, 0, sizeof(struct sigaction));
        act.sa_handler = f;
        if (-1==sigaction(sigNo, &act, NULL)) ERR("sigaction");
}

void child_work(int m) {
        //struct timespec t = {0, m*10000};
        srand(time(NULL)*getpid());
        int s, out;
        char name[32];
        char *buf;
        sprintf(name,"%d.txt",getpid());
        s = (10 + rand()%90)*1000;
        buf = malloc(s);
        if(!buf) ERR("malloc");
        buf = memset(buf, m, s);
        if((out=open(name,O_WRONLY|O_CREAT|O_TRUNC|O_APPEND,0777))<0)ERR("open");
        if((s=write(out,buf,s))<0) ERR("read");
        printf("n=%d , s=%d\n",m,s);
        
}

void create_children(int n, char** argv) {
        pid_t s;
        int m;
        for (n--;n>0;n--) {
                if((s=fork())<0) ERR("Fork:");
                if(!s) {
						m = atoi(argv[n]);
                        child_work(m);
                        exit(EXIT_SUCCESS);
                }
        }
}

void sigchld_handler(int sig) {
        pid_t pid;
        for(;;){
                pid=waitpid(0, NULL, WNOHANG);
                if(pid==0) return;
                if(pid<=0) {
                        if(errno==ECHILD) return;
                        ERR("waitpid");
                }
        }
}
void sig_handler(int sig) {
        last_signal = sig;
}

void parent_work() {
	struct timespec tk = {0, 10};
	sethandler(sig_handler,SIGALRM);
	alarm(1);
	while(last_signal!=SIGALRM) {
			nanosleep(&tk,NULL);
			if (kill(0, SIGUSR1)<0)ERR("kill");
		}
	
        
}

void usage(char *name){
	fprintf(stderr,"USAGE: %s m  p\n",name);
	exit(EXIT_FAILURE);
}


int main(int argc, char** argv) {
        //int i;
        int n;
        pid_t pid;
        n = argc;
        if(argc<2) usage(argv[0]);
		create_children(argc, argv);
		parent_work();
		for(;;){
			pid=waitpid(0, NULL, WNOHANG);
			if(pid>0) n--;
			if(0==pid) break;
			if(0>=pid) {
				if(ECHILD==errno) break;
				ERR("waitpid:");
			}
		}
        /*sethandler(sig_handler,SIGUSR1);
        pid_t pid;
        if((pid=fork())<0) ERR("fork");
        if(0==pid) child_work(m);
        else {
                parent_work(b,s*1024*1024,name);
                while(wait(NULL)>0);
        }*/
        return EXIT_SUCCESS;
}
