#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define SEED 77
#define DEFAULT_N 5
#define DEFAULT_T 2
#define DEFAULT_W 10
#define WRAP_TO_RANGE(random_number, min_incl, max_incl)\
((min_incl) + ((random_number) % ((max_incl) - (min_incl) + 1)))
#define NEXT_DOUBLE(seedptr) ((double) rand_r(seedptr) / (double) RAND_MAX)
#define NEXT_INT(seedptr) rand_r(seedptr)
#define ERR(source)                                                 \
   (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
    exit(EXIT_FAILURE))
// #define ELAPSED(start,end) ((end).tv_sec-(start).tv_sec)+(((end).tv_nsec -
// (start).tv_nsec) * 1.0e-9)

volatile sig_atomic_t last_signal = 0;

typedef struct timespec timespec_t;

typedef unsigned int UINT;
typedef struct argsSwaper {
   pthread_t tid;
   UINT seed;
   int id;
   int waitingTime;
   int *table;
   int tableSize;
   pthread_mutex_t *mxCell;
   bool *pQuitFlag;
   pthread_mutex_t *pmxQuitFlag;
   bool *pSwappersStartedFlag;
} argsSwaper_t;

typedef struct argsSignalHandler {
   pthread_t tid;
   sigset_t *pMask;
   bool *pQuitFlag;
   pthread_mutex_t *pmxQuitFlag;
   int *table;
   int tableSize;
   pthread_mutex_t *mxCell;
   int swappersCount;
   argsSwaper_t *argsSwappers;
   bool *pSwappersStartedFlag;
} argsSignalHandler_t;

void ReadArguments(int argc, char **argv, int *tableSize, int *swappersCount, int *waitingTime);
void make_swapers(argsSwaper_t *argsSwapers, int tableSize);
void thread_work(void *args);
void msleep(UINT milisec);
void* signal_handling(void*);
void sethandler(void (*f)(int), int sigNo);
void alarm_handler(int sig);

int main(int argc, char **argv) {
   int tableSize, swappersCount, waitingTime;
   bool quitFlag = false;
   bool swappersStartedFlag = false;
   // pthread_mutex_t mxQuitFlag = PTHREAD_MUTEX_INITIALIZER;
   pthread_mutex_t mxQuitFlag;
   pthread_mutex_init(&mxQuitFlag, NULL);
   ReadArguments(argc, argv, &tableSize, &swappersCount, &waitingTime);
   int table[tableSize];
   pthread_mutex_t mxCell[tableSize];
   for (int i = 0; i < tableSize; i++) {
      table[i] = tableSize - i;
      if (pthread_mutex_init(&mxCell[i], NULL))
         ERR("Couldn't initialize mutex!");
   }
   
   argsSwaper_t *argsSwappers =
           (argsSwaper_t *) malloc(sizeof(argsSwaper_t) * swappersCount);
   if (argsSwappers == NULL)
      ERR("Malloc error for throwers arguments!");
   // srand(time(NULL));
   srand(SEED);
   for (int i = 0; i < swappersCount; i++) {
      argsSwappers[i].seed = (UINT) rand();
      argsSwappers[i].id = i;
      argsSwappers[i].waitingTime = waitingTime;
      argsSwappers[i].table = table;
      argsSwappers[i].tableSize = tableSize;
      argsSwappers[i].mxCell = mxCell;
      argsSwappers[i].pQuitFlag = &quitFlag;
      argsSwappers[i].pmxQuitFlag = &mxQuitFlag;
      argsSwappers[i].pSwappersStartedFlag = &swappersStartedFlag;
   }
   
   sigset_t oldMask, newMask;
   sigemptyset(&newMask);
   sigaddset(&newMask, SIGINT);
   sigaddset(&newMask, SIGQUIT);
   // sigaddset(&newMask, SIGUSR1);
   // sigaddset(&newMask, SIGALRM);
   if (pthread_sigmask(SIG_BLOCK, &newMask, &oldMask)) ERR("SIG_BLOCK error");
   sethandler(SIG_IGN,SIGUSR1);
   argsSignalHandler_t argsSignal;
   argsSignal.pMask = &newMask;
   argsSignal.pQuitFlag = &quitFlag;
   argsSignal.pmxQuitFlag = &mxQuitFlag;
   argsSignal.table = table;
   argsSignal.tableSize = tableSize;
   argsSignal.mxCell = mxCell;
   argsSignal.swappersCount = swappersCount;
   argsSignal.argsSwappers = argsSwappers;
   argsSignal.pSwappersStartedFlag = &swappersStartedFlag;

   printf("\t\t      ");
   for (int i = 0; i < argsSwappers->tableSize; i++) printf("%2d ", i);
   printf("\n");
   if (pthread_create(&argsSignal.tid, NULL, signal_handling, &argsSignal))ERR("Couldn't create signal handling thread!");
   printf("main set alarm\n");
   // if (kill(0, SIGUSR1)<0)ERR("kill");
   make_swapers(argsSwappers, swappersCount);
   printf("main started threads\n");
   if(pthread_join(argsSignal.tid, NULL)) ERR("Can't join with 'signal handling' thread");
   printf("SignalHandlern joined");
   for (int i = 0; i < tableSize; i++) printf("%2d ", table[i]);
   free(argsSwappers);
   if (pthread_sigmask(SIG_UNBLOCK, &newMask, &oldMask)) ERR("SIG_UNBLOCK error");
   printf("main exits\n");
   exit(EXIT_SUCCESS);
}
void ReadArguments(int argc, char **argv, int *tableSize, int *swappersCount, int *waitingTime) {
   *tableSize = DEFAULT_N;
   *swappersCount = DEFAULT_T;
   *waitingTime = DEFAULT_W;
   if (argc >= 2) {
      *tableSize = atoi(argv[1]);
      if (*tableSize < 5 || *tableSize > 20) {
         printf("Invalid value for 'table size N'\n");
         exit(EXIT_FAILURE);
      }
   }
   if (argc >= 3) {
      *swappersCount = atoi(argv[2]);
      if (*swappersCount < 2 || *swappersCount > 40) {
         printf("Invalid value for 'swappers count T'\n");
         exit(EXIT_FAILURE);
      }
   }
   if (argc >= 4) {
      *waitingTime = atoi(argv[3]);
      if (*waitingTime < 10 || *waitingTime > 1000) {
         printf("Invalid value for 'waiting time W'\n");
         exit(EXIT_FAILURE);
      }
   }
}

void make_swapers(argsSwaper_t *argsSwapers, int swappersCount) {
   for (int i = 0; i < swappersCount; i++) {
      if (pthread_create(&argsSwapers[i].tid, NULL, (void *(*) (void *) ) thread_work,
                         &argsSwapers[i]))
         ERR("Couldn't create thread");
   }
   *argsSwapers[0].pSwappersStartedFlag = true;
}

void thread_work(void *voidArgs) {
   pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
   argsSwaper_t *args = (argsSwaper_t *) voidArgs;
   bool quitFlag = false;
   while (!quitFlag) {
      UINT k1 = NEXT_INT(&args->seed) % args->tableSize;
      UINT k2 = NEXT_INT(&args->seed) % args->tableSize;
      if (k1 == k2) continue;
      if (k2 < k1) {
         int tmp = k2;
         k2 = k1;
         k1 = tmp;
      }
      pthread_mutex_lock(&args->mxCell[k1]);
      msleep(200);
      pthread_mutex_lock(&args->mxCell[k2]);
      printf("Swaper %d k1=%d, k2=%d ", args->id, k1, k2);
      if (args->table[k1] > args->table[k2]) {
         int tmp = args->table[k2];
         args->table[k2] = args->table[k1];
         args->table[k1] = tmp;
      }
      for (int i = 0; i < args->tableSize; i++) printf("%2d ", args->table[i]);
      pthread_mutex_unlock(&args->mxCell[k1]);
      pthread_mutex_unlock(&args->mxCell[k2]);
      if (pthread_mutex_lock(args->pmxQuitFlag)) {
         printf("deadlock warning 3, continue...\n");
         continue;
      };
      quitFlag = *args->pQuitFlag;
      pthread_mutex_unlock(args->pmxQuitFlag);
      UINT sleeping_time = WRAP_TO_RANGE(NEXT_INT(&args->seed), 10, args->waitingTime);
      printf(" sleeps %u ms...\n", sleeping_time);
      msleep(sleeping_time);
   }
   printf("thread_work finished...\n");
   return;
}

void msleep(UINT milisec) {
   time_t sec = (int) (milisec / 1000.);
   milisec = milisec - (sec * 1000.);
   timespec_t req = {0};
   timespec_t req_rem = {0};
   req.tv_sec = sec;
   req.tv_nsec = milisec * 1000000L;
   // printf("1: %lds, %ldms, %lds, %ldms\n", req.tv_sec, req.tv_nsec/1000000,
   // req_rem.tv_sec, req_rem.tv_nsec/1000000);
   if (nanosleep(&req, &req_rem))
      ERR("nanosleep");
   // printf("2: %lds, %ldms, %lds, %ldms\n", req.tv_sec, req.tv_nsec/1000000,
   // req_rem.tv_sec, req_rem.tv_nsec/1000000);
}


void *signal_handling(void *voidArgs) {
   argsSignalHandler_t *args = (argsSignalHandler_t *) voidArgs;
   int signo;
   srand(time(NULL));
   for (;;) {
      if (sigwait(args->pMask, &signo)) ERR("sigwait failed.");
      switch (signo) {
      case SIGUSR1:
         printf("SIGUSR1\n");
         if (!args->pSwappersStartedFlag){
            make_swapers(args->argsSwappers, args->swappersCount);
            printf("signal_handling started threads\n");
         }
         break;
      case SIGUSR2:
         printf("SIGUSR2\n");
         for (int i = 0; i < args->tableSize; i++) printf("%2d ", args->table[i]);
         printf("\n");
         break;
      case SIGINT:
         printf("SIGINT\n");
         for (int i = 0; i < args->swappersCount; i++) {
            pthread_cancel(args->argsSwappers[i].tid);
            int err = pthread_join(args->argsSwappers[i].tid, NULL);
            if (err != 0)
               ERR("Can't join with a thread");
            printf("joined\n");
         }
         printf("swappers joined");
         return NULL;
      case SIGQUIT:
         printf("SIGQUIT\n");
         pthread_mutex_lock(args->pmxQuitFlag);
         *args->pQuitFlag = true;
         pthread_mutex_unlock(args->pmxQuitFlag);
         return NULL;
      case SIGALRM:
         printf("SIGALRM 1\n");
      default:
         printf("unexpected signal %d\n", signo);
         exit(1);
      }
   }
   return NULL;
}

void sethandler(void (*f)(int), int sigNo) {
   struct sigaction act;
   memset(&act, 0, sizeof(struct sigaction));
   act.sa_handler = f;
   if (-1 == sigaction(sigNo, &act, NULL)) ERR("sigaction");
}
void alarm_handler(int sig) {
   printf("[%d] received signal %d\n", getpid(), sig);
   last_signal = sig;
   if (SIGALRM == sig) {
      printf("SIGALRM 2\n");
      if (kill(0, SIGINT)<0)ERR("kill");
   }
}
