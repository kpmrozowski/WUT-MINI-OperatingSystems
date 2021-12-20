#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAXLINE 4096
#define DEFAULT_T 9
#define DEFAULT_N 11
#define NEXT_DOUBLE(seedptr) ((double) rand_r(seedptr) / (double) RAND_MAX)
#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))

typedef unsigned int UINT;
typedef struct argsThrower{
        pthread_t tid;
        UINT seed;
        int id;
        int *buildings;
        pthread_mutex_t *mxBuilding;
} argsThrower_t;

void ReadArguments(int argc, char** argv, int *simulationTime, int *buildingsCount);
void make_buildings(argsThrower_t *argsArray, int buildingsCount);
void* thread_work(void* args);

int main(int argc, char** argv) {
        int simulationTime, buildingsCount;
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
        exit(EXIT_SUCCESS);
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
        pthread_attr_t threadAttr;
        if(pthread_attr_init(&threadAttr)) ERR("Couldn't create pthread_attr_t");
        if(pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED)) ERR("Couldn't setdetachsatate on pthread_attr_t");
        for (int i = 0; i < buildingsCount; i++) {
                if(pthread_create(&argsArray[i].tid, &threadAttr, thread_work, &argsArray[i])) ERR("Couldn't create thread");
        }
        pthread_attr_destroy(&threadAttr);
}
void* thread_work(void* voidArgs) {
    argsThrower_t* args = voidArgs;
    printf("Created\n");
    while (1) {
        pthread_mutex_lock(args->mxBuilding);
        args->buildings[args->id] += 1;
        pthread_mutex_unlock(&args->mxBins[binno]);
        pthread_mutex_lock(args->pmxBallsThrown);
        (*args->pBallsThrown) += 1;
        pthread_mutex_unlock(args->pmxBallsThrown);
}
    return NULL;
}