#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))

volatile sig_atomic_t signal_SIG1 = 0;
volatile sig_atomic_t signal_SIG2 = 0;

void sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL)) ERR("sigaction");
}

void sig_handler(int sig) {
	if(SIGUSR1 == sig) {
		signal_SIG1 = sig;
		signal_SIG2 = 0;
	} else if (SIGUSR2 == sig) {
		signal_SIG1 = 0;
		signal_SIG2 = sig;
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

void child_work(int m, int p) {
	int count_SIG1 = 0;
	int count_SIG2 = 0;
	struct timespec t = {0, m*10000};
	while( 1 ) {
		for( int i = 0; i < p; i++ ) {
			nanosleep(&t,NULL);
			if(kill(getppid(),SIGUSR1))ERR("kill");
			++count_SIG1;
		}
		nanosleep(&t,NULL);
		if(kill(getppid(),SIGUSR2))ERR("kill");
		++count_SIG2;
		printf("[%d] sent %d SIGUSR2  and  %d SIGUSR1\n",getpid(), count_SIG2, count_SIG1);

	}
}


void parent_work(sigset_t oldmask) {
	int count = 0;
	int checks = 0;
	while( 1 ) {
		while( signal_SIG2 != SIGUSR2 ) {
			++checks;
			sigsuspend(&oldmask);
		}
		signal_SIG2 = 0;
		printf("[PARENT] received %d SIGUSR2, checks: %d\n", ++count, checks);
		
	}
}

void usage(char *name){
	fprintf(stderr,"USAGE: %s m  p\n",name);
	fprintf(stderr,"m - number of 1/1000 milliseconds between signals [1,999], i.e. one milisecond maximum\n");
	fprintf(stderr,"p - after p SIGUSR1 send one SIGUSER2  [1,999]\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
	int m,p;
	if(argc!=3) usage(argv[0]);
	m = atoi(argv[1]); p = atoi(argv[2]);
	if (m<=0 || m>99999 || p<=0 || p>49999)  usage(argv[0]); 
	sethandler(sigchld_handler,SIGCHLD);
	sethandler(sig_handler,SIGUSR1);
	sethandler(sig_handler,SIGUSR2);
	sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
	pid_t pid;
	if((pid=fork())<0) ERR("fork");
	if(0==pid) child_work(m,p);
	else {
		parent_work(oldmask);
		while(wait(NULL)>0);
	}
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	return EXIT_SUCCESS;
}

// soprun prog15 1 1000
// strace ./prog15 50000 3 (tylko usunac komentarze)
