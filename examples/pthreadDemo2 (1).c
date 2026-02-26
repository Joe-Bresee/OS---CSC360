#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

int value =0; 
void *runner(void *param); /* the thread */

int main(int argc, char *argv[])
{ //int pid;
  pthread_t tid; /* the thread identifier */
  pthread_attr_t attr; /* set of thread attributes */

 // pid = fork();
 
 // if (pid ==0){
  	/* get the default attributes */
  	pthread_attr_init(&attr);
  	/* create the thread */
  	pthread_create(&tid, &attr, runner, NULL);
  	/* now wait for the thread to exit */
  	pthread_join(tid,NULL);
  	printf("value = %d\n", value);
	
 // }
 // else if (pid >0){
//	wait(NULL);
//	printf("Parent value = %d \n", value);
//  }
  return 0;
}

/* The thread will begin control in this function */
void *runner(void *param)
{ int pid2;
  
  
  pid2=fork();
  if (pid2==0) {
       value =5;
       pthread_exit(0);
  }
  else {
      value = 3;
      pthread_exit(0);
  }
}

