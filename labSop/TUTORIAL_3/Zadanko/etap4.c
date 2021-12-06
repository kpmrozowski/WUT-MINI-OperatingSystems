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
	int *finished;
	pthread_mutex_t *mxFinished;
} argsDenominator_t;

void ReadArguments(int argc, char **argv, int *threadCount);
void *thread_function(void *args);

int main(int argc, char **argv)
{
	srand((unsigned)time(NULL));

	int threadCount, counter = 0, finished = 0;
	pthread_mutex_t mxCounter = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t mxFinished = PTHREAD_MUTEX_INITIALIZER;
	ReadArguments(argc, argv, &threadCount);

	printf("Thread count: %d\n", threadCount);

	argsDenominator_t *threads = (argsDenominator_t *)malloc(sizeof(argsDenominator_t) * threadCount);
	for (int i = 0; i < threadCount; i++)
	{
		threads[i].counter = &counter;
		threads[i].mxCounter = &mxCounter;
		threads[i].finished = &finished;
		threads[i].mxFinished = &mxFinished;
		pthread_create(&threads[i].tid, NULL, thread_function, &threads[i]);
	}

	struct timespec delay = {0, 1e8}, small_delay = {0, 1e7};

	while (true)
	{
		nanosleep(&delay, NULL);

		// wait for all threads
		while (true)
		{
			pthread_mutex_lock(&mxFinished);
			if (finished != threadCount)
			{
				pthread_mutex_unlock(&mxFinished);
				nanosleep(&small_delay, NULL);
			}
			else
			{
				finished = 0;
				break;
			}
		}

		// increment counter
		pthread_mutex_lock(&mxCounter);
		counter += 1;
		pthread_mutex_unlock(&mxCounter);
		pthread_mutex_unlock(&mxFinished);
	}

	for (int i = 0; i < threadCount; i++)
	{
		pthread_join(threads[i].tid, NULL);
	}

	return EXIT_SUCCESS;
}

void *thread_function(void *voidArgs)
{
	argsDenominator_t *args = voidArgs;
	unsigned int seed = (unsigned int)(args->tid);
	args->number = rand_r(&seed) % (MAX_NUMBER - MIN_NUMBER + 1) + MIN_NUMBER;

	int last = -1;

	while (true)
	{
		pthread_mutex_lock(args->mxCounter);
		if (*(args->counter) != last)
		{
			if (*(args->counter) % args->number == 0)
			{
				printf("%d jest podzielne przez %d\n", *(args->counter), args->number);
			}

			last = *(args->counter);

			pthread_mutex_lock(args->mxFinished);
			*(args->finished) += 1;
			pthread_mutex_unlock(args->mxFinished);
		}
		pthread_mutex_unlock(args->mxCounter);
	}

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