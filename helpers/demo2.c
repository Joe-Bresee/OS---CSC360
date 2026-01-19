/* Demos of fork(), exec(), waitpid(), kill() and backgound process 
   ps a
   ./inf tag interval
   "inf.c" prints the tag every (interval) seconds. The default maximum is 10 times. 
   You can change this number in "inf.c" based on your need. 
*/

#include <unistd.h>     // fork(), execvp()
#include <stdio.h>      // printf(), scanf(), setbuf(), perror()
#include <stdlib.h>     // malloc()
#include <sys/types.h>  // pid_t 
#include <sys/wait.h>   // waitpid()
#include <signal.h>     // kill(), SIGTERM, SIGKILL, SIGSTOP, SIGCONT
#include <errno.h>      // errno


int main(){

	pid_t pid;          // The ID of child process
	int status;

	char cont = 'y';   // y - continue to create new child process; n - exit.

	while (cont == 'y'){
		char *tag = (char *)malloc(sizeof(char));  
		char *interval = (char *)malloc(sizeof(char));  
		printf("Please input a character for tag: \n");
		scanf("%s",tag);
		printf("Please input a character for inteval: \n");
		scanf("%s",interval);
		
		printf("Start to create a child process... Calling fork()... \n");
		pid = fork();
			
		if(pid == 0){
			//Child process
			printf("...\n");
			char *argv_execvp[] = {"inf",tag,interval,NULL};
			if (execvp("./inf", argv_execvp) < 0)
				perror("Error on execvp");
			printf("Child finished.\n");  // This line won't work since exec() successfully runs and replaces the code of child process	
		}
		else if(pid > 0){

			//parent process
			printf("\nParent has created a new child with tag = %s and interval = %s.\n",tag,interval);
			printf("\nThe pid of the child process is: %d\n",pid);
			
			// int opts = WNOHANG | WUNTRACED | WCONTINUED;
			int opts = WUNTRACED|WCONTINUED;
                        int retVal;
	
			retVal = waitpid(pid, &status, opts);
			if (retVal == -1) { 
				perror("Fail at waitpid"); 
				exit(EXIT_FAILURE); 
			}
			// Macros below can be found by "$ man 2 waitpid"
    			if (WIFEXITED(status)) {
     				printf("Normal, status code=%d\n", WEXITSTATUS(status));  // Display the status code of child process
    			} else if (WIFSIGNALED(status)) {
    				printf("killed by signal %d\n", WTERMSIG(status));
    			} else if (WIFSTOPPED(status)) {
    				printf("stopped by signal %d\n", WSTOPSIG(status));
    			} else if (WIFCONTINUED(status)) {    
      				printf("continued\n");   
   			}
			
			sleep(10);
			//kill(pid, SIGKILL);	

		}
		else{  // Fail to create new process
			perror("\nFail to create a new process.\n");
		}
		
		printf("Do you want to continue (y/n)?\n ");
		setbuf(stdin, NULL);  // clear the buffer. You could try to remove this line to see the result
		scanf("%c", &cont);
	
	} // end of while loop
	
	printf("Parent process stops.\n");
	return 0;

}
