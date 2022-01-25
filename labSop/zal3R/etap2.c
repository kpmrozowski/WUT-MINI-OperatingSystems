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

typedef unsigned int UINT;
typedef struct argsSwaper {
   pthread_t tid;
   UINT seed;
   int id;
   int *table;
   int tableSize;
   pthread_mutex_t *mxCell;
} argsSwaper_t;

typedef struct timespec timespec_t;

void ReadArguments(int argc, char **argv, int *tableSize, int *swappersCount, int *waitingTime);
void make_swapers(argsSwaper_t *argsSwapers, int tableSize);
void thread_work(void *args);
void msleep(UINT milisec);

int main(int argc, char **argv) {
   int tableSize, swappersCount, waitingTime;
   ReadArguments(argc, argv, &tableSize, &swappersCount, &waitingTime);
   int table[tableSize];
   pthread_mutex_t mxCell[tableSize];
   for (int i = 0; i < tableSize; i++) {
      table[i] = tableSize - i;
      if (pthread_mutex_init(&mxCell[i], NULL))
         ERR("Couldn't initialize mutex!");
   }
   argsSwaper_t *args =
           (argsSwaper_t *) malloc(sizeof(argsSwaper_t) * swappersCount);
   if (args == NULL)
      ERR("Malloc error for throwers arguments!");
   // srand(time(NULL));
   srand(SEED);
   for (int i = 0; i < swappersCount; i++) {
      args[i].seed = (UINT) rand();
      args[i].id = i;
      args[i].table = table;
      args[i].tableSize = tableSize;
      args[i].mxCell = mxCell;
   }
   printf("\t\t      ");
   for (int i = 0; i < args->tableSize; i++) printf("%2d ", i);
   printf("\n");
   make_swapers(args, swappersCount);
   msleep(5000);
   for (int i = 0; i < swappersCount; i++) {
      pthread_cancel(args[i].tid);
      int err = pthread_join(args[i].tid, NULL);
      if (err != 0)
         ERR("Can't join with a thread");
      printf("joined\n");
   }
   for (int i = 0; i < tableSize; i++) printf("%2d ", table[i]);
   free(args);
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
}

void thread_work(void *voidArgs) {
   pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
   argsSwaper_t *args = (argsSwaper_t *) voidArgs;
   // printf("Created\n");
   while (1) {
      UINT k1 = NEXT_INT(&args->seed) % args->tableSize;
      UINT k2 = NEXT_INT(&args->seed) % args->tableSize;
      if (k1 == k2) continue;
      if (k2 < k1) {
         int tmp = k2;
         k2 = k1;
         k1 = tmp;
      }
      if (pthread_mutex_lock(&args->mxCell[k1])) {
         printf("deadlock warning 1, continue...\n");
         continue;
      };
      msleep(200);
      if (pthread_mutex_lock(&args->mxCell[k2])) {
         printf("deadlock warning 2, continue...\n");
         continue;
      };
      printf("Swaper %d k1=%d, k2=%d ", args->id, k1, k2);
      if (args->table[k1] > args->table[k2]) {
         int tmp = args->table[k2];
         args->table[k2] = args->table[k1];
         args->table[k1] = tmp;
      }
      for (int i = 0; i < args->tableSize; i++) printf("%2d ", args->table[i]);
      pthread_mutex_unlock(&args->mxCell[k1]);
      pthread_mutex_unlock(&args->mxCell[k2]);
      UINT sleeping_time = WRAP_TO_RANGE(NEXT_INT(&args->seed), 10, 1000);
      printf(" sleeps %u ms...\n", sleeping_time);
      msleep(sleeping_time);
   }
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