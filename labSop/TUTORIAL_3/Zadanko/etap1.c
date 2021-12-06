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

#define NEXT_DOUBLE(seedptr) ((double)rand_r(seedptr) / (double)RAND_MAX)
#define ELAPSED(start, end) ((end).tv_sec - (start).tv_sec) + (((end).tv_nsec - (start).tv_nsec) * 1.0e-9)
#define ERR(source) (perror(source),                                 \
					 fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
					 exit(EXIT_FAILURE))

#define DEFAULT_THREADS 10
#define MIN_NUMBER 2
#define MAX_NUMBER 100

typedef struct argsDenominator
{
	pthread_t tid;
	unsigned int number;
	int *counter;
	pthread_mutex_t *mxCounter;
} argsDenominator_t;

void ReadArguments(int argc, char **argv, int *threadCount);
void *thread_function(void *args);

int main(int argc, char **argv)
{
	srand((unsigned)time(NULL));

	int threadCount, counter = 0;
	ReadArguments(argc, argv, &threadCount);

	argsDenominator_t *threads = (argsDenominator_t *)malloc(sizeof(argsDenominator_t) * threadCount);
	for (int i = 0; i < threadCount; i++)
	{
		threads[i].mxCounter = NULL;
		threads[i].counter = &counter;
		pthread_create(&threads[i].tid, NULL, thread_function, &threads[i]);
	}

	for (int i = 0; i < threadCount; i++)
	{
		pthread_join(threads[i].tid, NULL);
	}

	return EXIT_SUCCESS;
}

void *thread_function(void *voidArgs)
{
	printf("*");
	return NULL;
}

void ReadArguments(int argc, char **argv, int *threadCount)
{
	*threadCount = DEFAULT_THREADS;

	if (argc >= 2)
	{
		*threadCount = atoi(argv[1]);
		if (*threadCount <= 0)
		{
			printf("Invalid value for 'thread count'");
			exit(EXIT_FAILURE);
		}
	}
}