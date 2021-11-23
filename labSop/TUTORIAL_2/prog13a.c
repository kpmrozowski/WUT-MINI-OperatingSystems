#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
					 perror(source), kill(0, SIGKILL),               \
					 exit(EXIT_FAILURE))

void child_work(int i)
{
	srand(time(NULL) * getpid());
	int t = 5 + rand() % (10 - 5 + 1);
	sleep(t);
	printf("PROCESS number %d with pid %d terminates\n", i, getpid());
}

void create_children(int n, pid_t *pids)
{
	pid_t s;
	for (n--; n >= 0; n--)
	{
		if ((s = fork()) < 0)
			ERR("Fork:");
		if (s == 0)
		{
			child_work(n);
			exit(EXIT_SUCCESS);
		}
		else
		{
			pids[n] = s;
		}
	}
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s 0<n\n", name);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int n;
	if (argc < 2)
		usage(argv[0]);
	n = atoi(argv[1]);
	if (n <= 0)
		usage(argv[0]);
	pid_t pids[n];
	create_children(n, pids);
	// wait for all children
	for (int i = 0; i < n; i++)
	{
		printf("Waiting for %d ...\n", pids[i]);
		if(-1 == waitpid(pids[i], NULL, 0))
		{
			ERR("waitpid");
		}
	}
	return EXIT_SUCCESS;
}
