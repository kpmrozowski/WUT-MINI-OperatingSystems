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

#define NAME_SIZE 20

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(exp) ({ \
  typeof (exp) _rc; \
  do { \
    _rc = (exp); \
  } while (_rc == -1 && errno == EINTR); \
  _rc; })
#endif

#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
                     perror(source), kill(0, SIGKILL),               \
                     exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s [n] \n", name);
    fprintf(stderr, "n - numbers written to files\n");
    exit(EXIT_FAILURE);
}

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
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

//////////////////////////////////////////////////////////////////////////

volatile sig_atomic_t sig_count = 0;
volatile sig_atomic_t last_sig = 0;

void handle_alarm(int sig)
{
    last_sig = sig;
}

void handle_alarm_parent(int sig)
{
    printf("[PARENT] Sent %d signals\n", sig_count);
    while (wait(NULL) > 0)
        ;
    exit(EXIT_SUCCESS);
}

void handle_sigusr1(int sig)
{
    sig_count++;
    last_sig = sig;
}

void child_work(int n, sigset_t oldmask)
{
    srand(getpid());
    int s = (10 + rand() % (100 - 10 + 1)) * 1024;
    int out_file;
    char name[NAME_SIZE];

    char *buf = malloc(sizeof(char) * s);
    if (!buf)
        ERR("malloc");

    if (!memset(buf, n + '0', s))
        ERR("memset");

    int written = 0;

    sprintf(name, "%d.txt", getpid());

    if ((out_file = TEMP_FAILURE_RETRY(open(name, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777))) < 0)
        ERR("open");

    alarm(1);
    printf("Starting: n = %d, s = %d\n", n, s);
    while (1)
    {
        sigsuspend(&oldmask);
        if (last_sig == SIGUSR1)
        {
            if (bulk_write(out_file, buf, s) < 0)
                ERR("bulk_write");
            written++;
        }
        else
        {
            printf("Writing remaining %d (written: %d, counter: %d)\n", sig_count - written, written, sig_count);
            while (written < sig_count)
            {
                if (bulk_write(out_file, buf, 1) < 0)
                    ERR("bulk_write");
                written++;
            }
            if (TEMP_FAILURE_RETRY(close(out_file)) < 0)
                ERR("close");

            free(buf);

            printf("%d.txt should be of size %d\n", getpid(), written * s);
            exit(EXIT_SUCCESS);
        }
    }
}

void parent_work()
{
    struct timespec t = {0, 10000000};
    while (1)
    {
        nanosleep(&t, NULL);
        if (kill(0, SIGUSR1))
            ERR("kill");
        sig_count++;
    }
}

int main(int argc, char **argv)
{
    // ignore SIGUSR1
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    for (int i = 1; i < argc; i++)
    {
        pid_t pid;
        if (0 == (pid = fork()))
        {
            int n = atoi(argv[i]);
            if (n < 0 || n > 9)
                usage(argv[0]);

            sethandler(handle_sigusr1, SIGUSR1);
            sethandler(handle_alarm, SIGALRM);

            child_work(n, oldmask);
            exit(EXIT_SUCCESS);
        }
        else if (pid < 0)
        {
            ERR("fork");
        }
    }

    sethandler(handle_alarm_parent, SIGALRM);
    alarm(1);
    printf("Starting sending!\n");
    parent_work();

    return EXIT_SUCCESS;
}