
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int value =0; 
void *runner(void *param); /* the thread */

int main(int argc, char *argv[])
{ int pid;
  pthread_t tid; /* the thread identifier */
  pthread_attr_t attr; /* set of thread attributes */

  pid = fork();
 
  if (pid ==0){
        value++;
  	/* get the default attributes */
  	pthread_attr_init(&attr);
  	/* create the thread */
  	pthread_create(&tid, &attr, runner, NULL);
  	/* now wait for the thread to exit */
  	pthread_join(tid,NULL); 
  	printf("Child value = %d\n", value);
	
  }
 else if (pid >0){
       /* get the default attributes */
        pthread_attr_init(&attr);
        /* create the thread */
        pthread_create(&tid, &attr, runner, NULL);
        /* now wait for the thread to exit */
        pthread_join(tid,NULL);
	wait(NULL);
	printf("Parent value = %d \n", value);
  }
  return 0;
}

/* The thread will begin control in this function */
void *runner(void *param)
{ 
  value++;
  pthread_exit(0);
}

