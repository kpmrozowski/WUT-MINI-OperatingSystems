#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

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

void sig_SIG1_handler(int sig) {
	signal_SIG1 = sig;
}

void sig_SIG2_handler(int sig) {
	signal_SIG2 = sig;
	printf("[%d] I got SIGUSR2\n", getpid());
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

void child_work() {
	if(kill(getppid(),SIGUSR1))ERR("kill");
	printf("[%d]=> last child\n", getppid());
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

int make_file(int depth, int size) {
	int out;
	int LEN = 3 * sizeof(pid_t) + 2;
	char * mypid = malloc(LEN + 1);
	if(!mypid) ERR("malloc mypid");
	snprintf(mypid, LEN, "PID_%d.txt", getpid());
	
	ssize_t count;
	char *buf=malloc(size);
	if(!buf) ERR("malloc");
    if (!memset(buf, depth + '0', size)) ERR("memset");
	// int written = 0;

	if((out=open(mypid,O_WRONLY|O_CREAT|O_TRUNC|O_APPEND,0777))<0) ERR("open");

	if((count=bulk_write(out, buf, 1))<0) ERR("write");
	if(close(out))ERR("close");
	
	free(mypid);
	free(buf);
	return 0;
}

void parent_work(sigset_t oldmask) {
	int count = 0;
	int checks = 0;
    sethandler(SIG_IGN,SIGUSR2);
	while( 1 ) {
		while( signal_SIG1 != SIGUSR1 ) {
			++checks;
			sigsuspend(&oldmask);
		}
		signal_SIG1 = 0;
		printf("[PARENT] received %d SIGUSR1, checks: %d\n", ++count, checks);
		struct timespec t = {0, 200000};
		while( 1 ) {
			nanosleep(&t,NULL);
			if(kill(getppid(),SIGUSR2)) ERR("kill");
		}
		
	}
}

void spawn_children(int how_much_more, int depth, int size, sigset_t oldmask) {
	pid_t pid;
	if((pid=fork())<0) ERR("fork");
	if(0==pid) {
		sethandler(sig_SIG2_handler,SIGUSR2);
    	sethandler(SIG_IGN,SIGUSR1);
		printf("[%d.txt] child, depth %d\n", getppid(), depth + 1);
		if (make_file(depth, size) < 0) ERR("make_file");
		if (how_much_more == 0) {
			child_work();
		} else {
			spawn_children(how_much_more - 1, depth + 1, size, oldmask);
		}
	} else {
		sethandler(sig_SIG1_handler,SIGUSR1);
		parent_work(oldmask);
		while(wait(NULL)>0);
	}
}

void usage(char *name){
	fprintf(stderr,"USAGE: %s m  p\n",name);
	fprintf(stderr,"d - tree depth (0,100>\n");
	fprintf(stderr,"s - size of of blocks <100,1000> in B\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
	printf("[%d.txt] parent\n", getppid());
	int d,s;
	if(argc!=3) usage(argv[0]);
	d = atoi(argv[1]); s = atoi(argv[2]);
	if (d<=0 || d>100 || s<100 || s>1000)  usage(argv[0]); 
	sethandler(sigchld_handler,SIGCHLD);
	sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
	int depth = 0;
	spawn_children(d-1, depth, s, oldmask);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	return EXIT_SUCCESS;
}

// soprun prog 1 1000
// strace ./prog 50000 3 (tylko usunac komentarze)
