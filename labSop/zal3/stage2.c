#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>

#define MAXLINE 4096
#define DEFAULT_T 9
#define DEFAULT_N 11
#define NEXT_DOUBLE(seedptr) ((double) rand_r(seedptr) / (double) RAND_MAX)
#define NEXT_INT(seedptr) rand_r(seedptr)
#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))
#define ELAPSED(start,end) ((end).tv_sec-(start).tv_sec)+(((end).tv_nsec - (start).tv_nsec) * 1.0e-9)

typedef unsigned int UINT;
typedef struct argsThrower{
        pthread_t tid;
        UINT seed;
        int id;
        int *buildings;
        pthread_mutex_t *mxBuilding;
} argsThrower_t;

typedef struct timespec timespec_t;

void ReadArguments(int argc, char** argv, int *simulationTime, int *buildingsCount);
void make_buildings(argsThrower_t *argsArray, int buildingsCount);
void thread_work(void* args);
void msleep(UINT milisec);
void mainThreadWork(    int *simulationTime, 
                        int *buildingsCount, 
                        int *kongPosition, 
                        int* buildings, 
                        pthread_mutex_t* mxBuilding);
void sig_handler();

int main(int argc, char** argv) {
        int simulationTime, buildingsCount, kongPosition=0;
        ReadArguments(argc, argv, &simulationTime, &buildingsCount);
        int buildings[DEFAULT_N];
        pthread_mutex_t mxBuilding[DEFAULT_N];
        for (int i =0; i < DEFAULT_N; i++) {
                buildings[i] = 0;
                if(pthread_mutex_init(&mxBuilding[i], NULL))ERR("Couldn't initialize mutex!");
        }
        argsThrower_t* args = (argsThrower_t*) malloc(sizeof(argsThrower_t) * buildingsCount);
        if (args == NULL) ERR("Malloc error for throwers arguments!");
        srand(time(NULL));
        for (int i = 0; i < buildingsCount; i++) {
                args[i].seed = (UINT) rand();
                args[i].id = i;
                args[i].buildings = buildings;
                args[i].mxBuilding = mxBuilding;
        }
        make_buildings(args, buildingsCount);
        mainThreadWork( &simulationTime,
                        &buildingsCount,
                        &kongPosition, 
                        buildings, 
                        mxBuilding);
        // sethandler(sig_handler,SIGALRM);
        for (int i = 0; i < buildingsCount; i++) {
                pthread_cancel(args[i].tid);
                int err = pthread_join(args[i].tid, NULL);
                if (err != 0) ERR("Can't join with a thread");
                printf("joined\n");
        }
        printf("final buildings:\t");
        for (int i = 0; i < buildingsCount; i++) printf("%d   ", buildings[i]);
        printf("\n");
        printf("final kong position: \t%d\n", kongPosition);
        printf("\n");
        
        // free(buildings);
        // free(args);
        // free(mxBuilding);
        // free(argv);
                
        exit(EXIT_SUCCESS);
}
void alarm_handler() {
        ;
}
void mainThreadWork(    int *simulationTime, 
                        int *buildingsCount, 
                        int *kongPosition, 
                        int* buildings, 
                        pthread_mutex_t* mxBuilding) {
        bool jumped = false;
        int simTime = 0;
        for (;;) {
                if (*kongPosition == *buildingsCount || simTime == *simulationTime) break;
                sleep(1);
                pthread_mutex_lock(&mxBuilding[*kongPosition]);
                pthread_mutex_lock(&mxBuilding[*kongPosition+1]);
                if (buildings[*kongPosition+1] >= buildings[*kongPosition]) jumped = true;
                pthread_mutex_unlock(&mxBuilding[*kongPosition]);
                pthread_mutex_unlock(&mxBuilding[*kongPosition+1]);
                printf("kong position: %d\n", *kongPosition);
                if (jumped) ++(*kongPosition);
                printf("buildings:\t");
                for (int i = 0; i < *buildingsCount; i++)  {
                        pthread_mutex_lock(&mxBuilding[i]);
                        printf("%d   ", buildings[i]);
                        pthread_mutex_unlock(&mxBuilding[i]);
                }
                printf("\n");
                ++simTime;
        }
}
void ReadArguments(int argc, char** argv, int *simulationTime, int *buildingsCount) {
        *simulationTime = DEFAULT_T;
        *buildingsCount = DEFAULT_N;
        if (argc >= 2) {
                *simulationTime = atoi(argv[1]);
                if (*simulationTime < 5) {
                        printf("Invalid value for 'simulation time'\n");
                        exit(EXIT_FAILURE);
                }
        }
        if (argc >= 3) {
                *buildingsCount = atoi(argv[2]);
                if (*buildingsCount <= 2) {
                        printf("Invalid value for 'buildings count'\n");
                        exit(EXIT_FAILURE);
                }
        }
}
void make_buildings(argsThrower_t *argsArray, int buildingsCount) {
        for (int i = 0; i < buildingsCount; i++) {
                if(pthread_create(&argsArray[i].tid, NULL, thread_work, &argsArray[i])) ERR("Couldn't create thread");
        }
}
void thread_work(void* voidArgs) {
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
        argsThrower_t* args = voidArgs;
        printf("Created\n");
        while (1) {
                if(pthread_mutex_trylock(&args->mxBuilding[args->id])) {
                        printf("deadlock warning, continue...\n");
                        continue;
                };
                args->buildings[args->id] += 1;
                pthread_mutex_unlock(&args->mxBuilding[args->id]);
                msleep(NEXT_INT(&args->seed) % 401 + 100);
        }
        return;
}

void msleep(UINT milisec) {
    time_t sec= (int)(milisec/1000);
    milisec = milisec - (sec*1000);
    timespec_t req= {0};
    req.tv_sec = sec;
    req.tv_nsec = milisec * 1000000L;
    if(nanosleep(&req,&req)) ERR("nanosleep");
}