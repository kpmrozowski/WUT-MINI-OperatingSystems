#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#define MAXLINE 4096
#define DEFAULT_GRASS 100
#define DEFAULT_TIME 10
#define ELAPSED(start,end) ((end).tv_sec-(start).tv_sec)+(((end).tv_nsec - (start).tv_nsec) * 1.0e-9)
#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))

typedef unsigned int UINT;
typedef struct timespec timespec_t;
typedef struct grassstruct
{
	pthread_t tid;
	int h;
	UINT seed;
} grassx;
void ReadArguments(int argc, char** argv, int *grassCount, int *timex);
void* grass(void* arg);
void msleep(UINT milisec);

int main(int argc, char** argv) {
        /*int studentsCount;
        ReadArguments(argc, argv, &studentsCount);
        yearCounters_t counters = {
                .values = { 0, 0, 0, 0 },
                .mxCounters = {
                                PTHREAD_MUTEX_INITIALIZER,
                                PTHREAD_MUTEX_INITIALIZER,
                                PTHREAD_MUTEX_INITIALIZER,
                                PTHREAD_MUTEX_INITIALIZER}
        };
        studentsList_t studentsList;
        studentsList.count = studentsCount;
        studentsList.present = studentsCount;
        studentsList.thStudents = (pthread_t*) malloc(sizeof(pthread_t) * studentsCount);
        studentsList.removed = (bool*) malloc(sizeof(bool) * studentsCount);
        if (studentsList.thStudents == NULL || studentsList.removed == NULL)
                ERR("Failed to allocate memory for 'students list'!");
        for (int i = 0; i < studentsCount; i++) studentsList.removed[i] = false;
        for (int i = 0; i < studentsCount; i++)
                if(pthread_create(&studentsList.thStudents[i], NULL, student_life, &counters)) ERR("Failed to create student thread!");
        srand(time(NULL));
        timespec_t start, current;
        if (clock_gettime(CLOCK_REALTIME, &start)) ERR("Failed to retrieve time!");
        do {
                msleep(rand() % 201 + 100);
                if (clock_gettime(CLOCK_REALTIME, &current)) ERR("Failed to retrieve time!");
                kick_student(&studentsList);
        }
        while (ELAPSED(start, current) < 4.0);
        for (int i = 0; i < studentsCount; i++)
                if(pthread_join(studentsList.thStudents[i], NULL)) ERR("Failed to join with a student thread!");
        printf(" First year: %d\n", counters.values[0]);
        printf("Second year: %d\n", counters.values[1]);
        printf(" Third year: %d\n", counters.values[2]);
        printf("  Engineers: %d\n", counters.values[3]);
        free(studentsList.removed);
        free(studentsList.thStudents);*/
        int grassCount;
        int timex;
        srand(time(NULL));
        ReadArguments(argc, argv, &grassCount, &timex);
        grassx* arg = (grassx*)malloc(sizeof(grassx)*grassCount);
        
        for(int i = 0; i < grassCount; i++)
        {
			arg[i].seed = (UINT) rand();
			if(pthread_create(&(arg[i].tid),NULL,grass,&arg[i])) ERR("Failed to create grass thread!");
		}
		for (int i = 0; i < timex; i++)
		{
			for (int j = 0; j < grassCount; j++)
			{
				if (arg[j].h>10) arg[j].h=10;
				printf("%d ", arg[j].h);
			}
			printf("\n");
			sleep(1);
		}
			
		for(int i = 0; i < grassCount; i++)
		{
			pthread_cancel(arg[i].tid);
			if(pthread_join(arg[i].tid, NULL)) ERR("Failed to join a grass thread!");
		}	
        exit(EXIT_SUCCESS);
}
void ReadArguments(int argc, char** argv, int *grassCount, int *timex) {
        *grassCount = DEFAULT_GRASS;
        *timex  = DEFAULT_TIME;
        if (argc == 2) {
                *grassCount = atoi(argv[1]);
                if (*grassCount <= 0) {
                        printf("Invalid value for 'grassCount'");
                        exit(EXIT_FAILURE);
                }
			}
			else
			{
				*grassCount = atoi(argv[2]);
			if (*grassCount <= 0) {
					printf("Invalid value for 'grassCount'");
					exit(EXIT_FAILURE);
			   }
				*timex = atoi(argv[1]);
				if (*timex <= 0) {
					printf("Invalid value for 'timex'");
					exit(EXIT_FAILURE);
			   }
        }
}

void* grass(void* arg)
{
	//grassx* args = arg;
	while (true)
	{
		sleep(0.1);
	}
	return 0;
}
void msleep(UINT milisec) {
    time_t sec= (int)(milisec/1000);
    milisec = milisec - (sec*1000);
    timespec_t req= {0};
    req.tv_sec = sec;
    req.tv_nsec = milisec * 1000000L;
    if(nanosleep(&req,&req)) ERR("nanosleep");
}
       
