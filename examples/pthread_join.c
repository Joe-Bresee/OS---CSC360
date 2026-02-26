#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>  // intptr_t
#include <time.h>

void *print_msg(void *ptr);

int main() {
	
     pthread_t thread1, thread2;
     const char *msg1 = "I'm thread 1.";
     const char *msg2 = "I'm thread 2.";
     int  ret_crt1, ret_crt2;
	 void * ret_thrd1, * ret_thrd2;

    /* Create independent threads each of which will execute function */

     ret_crt1 = pthread_create( &thread1, NULL, print_msg, (void *) msg1); 
     if(ret_crt1) /* check the return value of pthread_create() */
     {
         fprintf(stderr,"Error - pthread_create() return code: %d\n",ret_crt1);
         exit(EXIT_FAILURE);
     }
	 
	 pthread_join(thread1, &ret_thrd1);
	 printf("Thread 1 returns: %ld\n",(intptr_t)ret_thrd1);
	 
	 sleep(5);
	 
     ret_crt2 = pthread_create( &thread2, NULL, print_msg, (void *) msg2);
     if(ret_crt2) /* check the return value of pthread_create() */
     {
         fprintf(stderr,"Error - pthread_create() return code: %d\n",ret_crt2);
         exit(EXIT_FAILURE);
     }
	 
	 /* check the returned values
     printf("returned value of creating thread 1: %d\n", ret_crt1);
     printf("returned value of creating thread 2: %d\n", ret_crt2);
	 */
	 
     /* Wait till threads are complete before main continues. Unless we  */
     /* wait we run the risk of executing an exit which will terminate   */
     /* the process and all threads before the threads have completed.   */

     pthread_join(thread2, &ret_thrd2); 
	 printf("Thread 2 returns: %ld\n",(intptr_t)ret_thrd2);
	 
     exit(EXIT_SUCCESS);

	 return 0;
}

void *print_msg(void *ptr)
{
	 char *msg;
	 msg = (char *)ptr;
     printf("%s \n", msg);
	 
	 /*return a random number*/
	 srand(time(NULL));
	 intptr_t randNo = rand() % 100;
	 return (void *)randNo; 
}
