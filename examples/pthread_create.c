#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>  //intptr_t
#include <time.h>

/* create thread argument struct for thr_func() */
typedef struct _customer{
	int id;
	int priority;
	int time; //e.g., service time
}customer;

/* thread function */
void *print_function(void *ptr){
	customer * cPtr = (customer *)ptr;
	sleep(cPtr->time);
	printf("Service customer %2d. for %2d seconds\n", cPtr->id, cPtr->time);

	return NULL;
}

int main()
{
	int num = 10; //customer number
	customer cusArray[num]; //customer array
	pthread_t thrdArray[num]; //thread array
	int i, rc, randNo;
	
	/* create threads */
	for (i = 0; i < num; ++i) {
		//srand(time(NULL));
		randNo = rand() % 10 + 1;
		cusArray[i].id = i;
		cusArray[i].time = randNo;
		if ((rc = pthread_create(&thrdArray[i], NULL, print_function, &cusArray[i]))) {
			fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
			return EXIT_FAILURE;
		}
	}
	/* block until all threads complete 
	for (i = 0; i < num; ++i) {
		pthread_join(thrdArray[i], NULL);
	}*/
	sleep(10);//wait for customers leaving
	return EXIT_SUCCESS;
}

