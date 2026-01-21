#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "linked_list.h"

Node* head = NULL;

/*
ASK: my check_bg_jobs implementation and use?
ASK: do I submit just main.c or linkedlist.c/.h too? my implementation of linkedlist.c may differ.
ASK: my utime is always 0
*/

void func_BG(char **cmd){
  
  if (cmd[1] == NULL) {
    printf("No bg names provided, not starting any background processes.\n");
    return;
  }

  char **args = &cmd[1];

  pid_t pid = fork();

  // child process
  if (pid == 0) {
    execvp(args[0], args);
    // func returns only if child execvp fails
    perror("execvp failed");
    exit(EXIT_FAILURE);
    // parent process
  } else if (pid > 0) {
    printf("Started bg process %d\n", pid);
    head = add_newNode(head, pid, cmd[1]);
  }
  else {
    perror("fork failed");
  }

  return;
}


void func_BGlist(char **cmd){printList(head);}


void check_bg_jobs(){
  int status;
  pid_t pid;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {

  // inform user of termination signal type
  if (WIFSIGNALED(status)) {
    printf("Process %d terminated by signal %d\n", pid, WTERMSIG(status));
  } else if (WIFEXITED(status)) {
    printf("Process %d terminated normally, exit %d\n", pid, WEXITSTATUS(status));
  }

  // delete just terminated node
  if (PifExist(head, pid)){
    head = deleteNode(head, pid);
    }
  }
}


void func_BGkill(char * str_pid){
	if (str_pid == NULL) {
		printf("Error: No PID provided\n");
		return;
	}
	
	pid_t pid = atoi(str_pid);
  
	
	if (PifExist(head, pid)) {
		if (kill(pid, SIGTERM) == 0) {
			printf("Sent SIGTERM to process %d successfully\n", pid);

      // allow small time for OS to update child's status 
      /*I tried immediately running check_bg_jobs and it would not update in time, therefore check_bg_jobs would see and kill
      the process only after the next user input, which could be bglist, which would then incorrectly show the killed process's pid
      and then immediately after show the terminationt type. I thought the most in-scope solution for this for the assignment would
      be this small delay.*/
      usleep(10000);
      check_bg_jobs();
		} else {
			perror("Failed to send SIGTERM to process");
		}
	} else {
		printf("Process %d not found in background list\n", pid);
	}
}


void func_BGstop(char * str_pid){
	if (str_pid == NULL) {
		printf("Error: No PID provided\n");
		return;
	}
	
	pid_t pid = atoi(str_pid);
	
	if (PifExist(head, pid)) {
		if (kill(pid, SIGSTOP) == 0) {
			printf("SIGSTOP sent to process %d successfully\n", pid);
		} else {
			perror("Failed to send SIGSTOP to process");
		}
	} else {
		printf("Process %d not found in background list\n", pid);
	}
}


void func_BGstart(char * str_pid){
	if (str_pid == NULL) {
		printf("Error: No PID provided\n");
		return;
	}
	
	pid_t pid = atoi(str_pid);
	
	if (PifExist(head, pid)) {
		if (kill(pid, SIGCONT) == 0) {
			printf("Sent SIGCONT to process %d successfully\n", pid);
		} else {
			perror("Failed to send SIGCONT to process");
		}
	} else {
		printf("Process %d not found in background list\n", pid);
	}
}

// Helper function to read /proc/[pid]/stat file
int read_stat_file(pid_t pid, char *comm, char *state, unsigned long *utime, unsigned long *stime, long *rss) {
    char statpath[256];
    FILE *file;
    
    snprintf(statpath, sizeof(statpath), "/proc/%d/stat", pid);
    file = fopen(statpath, "r");
    if (!file) {
        return -1; // Error opening file
    }
    
    int result = fscanf(
        file,
        "%*d " //1
        "(%255[^)]) %c " //comm:2, state: 3
        "%*d %*d %*d %*d %*d %*u %*u %*u %*u %*lu" //4-13
        "%lu %lu "  //utime: 14, stime: 15
        "%*d %*d %*d %*d %*d %*d %*ld %*ld " //16-23
        "%ld ", //rss: 24
        comm, state, utime, stime, rss //comm: 2, state: 3, utime: 14, stime: 15, rss: 24
    );
    fclose(file);
    
    // return if all values were successfully obtained
    if (result == 5) {return 0;}
    return -1;
}

// Helper function to read /proc/[pid]/status file  
int read_status_file(pid_t pid, unsigned long *voluntary, unsigned long *nonvoluntary) {
    char statuspath[256];
    char line[256];
    FILE *file;
    int found_voluntary = 0, found_nonvoluntary = 0;
    
    snprintf(statuspath, sizeof(statuspath), "/proc/%d/status", pid);
    file = fopen(statuspath, "r");
    if (!file) {
        return -1; // Error opening file
    }
    
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "voluntary_ctxt_switches: %lu", voluntary) == 1) {
            found_voluntary = 1;
        }
        if (sscanf(line, "nonvoluntary_ctxt_switches: %lu", nonvoluntary) == 1) {
            found_nonvoluntary = 1;
        }
    }
    fclose(file);
    
    // return if all values were successfully obtained
    if (found_voluntary && found_nonvoluntary) {return 0;}
    return -1;
}

void func_pstat(char * str_pid){
	if (str_pid == NULL) {
		printf("Error: No PID provided\n");
		return;
	}
	
	pid_t pid = atoi(str_pid);
	
	if (!PifExist(head, pid)) {
		printf("Process %d not found\n", pid);
		return;
	}

    // Variables to hold statistics
    char comm[256];
    char state;
    unsigned long utime, stime;
    long rss;
    unsigned long voluntary, nonvoluntary;
    
    // Read from /proc/[pid]/stat
    if (read_stat_file(pid, comm, &state, &utime, &stime, &rss) != 0) {
        printf("Error: Process %d statistics not found.\n", pid);
        return;
    }
    
    // Read from /proc/[pid]/status
    if (read_status_file(pid, &voluntary, &nonvoluntary) != 0) {
        printf("Error: Process %d status statistics not found.\n", pid);
        return;
    }

    // convert utime/stime to seconds
    double utime_sec = (double)utime / sysconf(_SC_CLK_TCK);
    double stime_sec = (double)stime / sysconf(_SC_CLK_TCK);
    
    // Print formatted process statistics
    printf("<=== PID %d statistics ===>\n\n", pid);
    printf("comm: %s\n"
           "state: %c\n"
           "utime: %.2f seconds\n"
           "stime: %.2f seconds\n"
           "rss: %ld\n"
           "voluntary context switch count: %lu\n"
           "nonvoluntary context switch count: %lu\n",
           comm, state, utime_sec, stime_sec, rss, voluntary, nonvoluntary);
}

 
int main(){
    char user_input_str[50];
    while (true) {

      // check for terminated processes
      check_bg_jobs();
      printf("Pman: > ");
      fgets(user_input_str, 50, stdin);
      printf("User input: %s \n", user_input_str);
      char * ptr = strtok(user_input_str, " \n");
      if(ptr == NULL){
        continue;
      }
      char * lst[50];
      int index = 0;
      lst[index] = ptr;
      index++;
      while(ptr != NULL){
        ptr = strtok(NULL, " \n");
        lst[index]=ptr;
        index++;
      }
      if (strcmp("bg",lst[0]) == 0){
        func_BG(lst);
      } else if (strcmp("bglist",lst[0]) == 0) {
        func_BGlist(lst);
      } else if (strcmp("bgkill",lst[0]) == 0) {
        func_BGkill(lst[1]);
      } else if (strcmp("bgstop",lst[0]) == 0) {
        func_BGstop(lst[1]);
      } else if (strcmp("bgstart",lst[0]) == 0) {
        func_BGstart(lst[1]);
      } else if (strcmp("pstat",lst[0]) == 0) {
        func_pstat(lst[1]);
      } else if (strcmp("q",lst[0]) == 0) {
        while (head) {
          head = deleteNode(head, head->pid);
        }
        printf("Bye Bye \n");
        exit(0);
      } else {
        printf("command not found\n");
      }
    }

  return 0;
}

