#include "bufferSemaphore.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define TRUE 1
//Q: what is the expected behavior? 

buffer_item buffer[BUFFER_SIZE];
pthread_mutex_t mutex;
sem_t empty;
sem_t full;

int insertPointer = 0, removePointer = 0;

void *producer(void *param);
void *consumer(void *param);

int insert_item(buffer_item item)
{
	sem_wait(&empty);
	
	pthread_mutex_lock(&mutex);

	buffer[insertPointer++] = item;
	insertPointer = insertPointer % BUFFER_SIZE;
/*
 The following two line of codes violate the "inside" rule. 
 Technically, this is legal and safe in most POSIX implementations, 
 but it has a subtle negative consequence: The Priority Inversion Risk.
*/
	pthread_mutex_unlock(&mutex);

        sem_post(&full);

	return 0;
}

int remove_item(buffer_item *item)
{
	sem_wait(&full);

	pthread_mutex_lock(&mutex);

	*item = buffer[removePointer];
	buffer[removePointer++] = -1;
	removePointer = removePointer % BUFFER_SIZE;

/*
 The following two line of codes violate the "inside" rule.
 Technically, this is legal and safe in most POSIX implementations,
 but it has a subtle negative consequence: The Priority Inversion Risk.
*/

	pthread_mutex_unlock(&mutex);

	sem_post(&empty);

	return 0;
}


int main(int argc, char *argv[])
{
	int sleepTime, producerThreads, consumerThreads;
	int i, j;

	if(argc != 4)
	{
		fprintf(stderr, "Useage: <Main thread sleep time> <# of producer threads> <# of consumer threads>\n");
		return -1;
	}

	sleepTime = atoi(argv[1]);
	producerThreads = atoi(argv[2]);
	consumerThreads = atoi(argv[3]);

	//Initialize the the locks
	printf("pthread_mutex_init: %d\n",pthread_mutex_init(&mutex, NULL));
	sem_init(&empty, 0, 1); //Initially, empty value should be larger than 0 
	sem_init(&full, 0, 0);
        printf("-------Initializing finished------------\n");
	srand(time(0));

	//Create the producer and consumer threads
	for(i = 0; i < producerThreads; i++)
	{
		pthread_t tid;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_create(&tid, &attr, producer, NULL);
	}

	for(j = 0; j < consumerThreads; j++)
	{
		pthread_t tid;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_create(&tid, &attr, consumer, NULL);
	}

	//Sleep for user specified time
	sleep(sleepTime);
	return 0;
}

void *producer(void *param)
{
	buffer_item random;

	while(TRUE)
	{
		random = rand();

		if(insert_item(random))
			fprintf(stderr, "Error");

		printf("Producer produced %d \n", random);

	}

}

void *consumer(void *param)
{
	buffer_item random;

	while(TRUE)
	{

		if(remove_item(&random))
			fprintf(stderr, "Error Consuming");
		else
			printf("Consumer consumed %d \n", random);
	}
}
