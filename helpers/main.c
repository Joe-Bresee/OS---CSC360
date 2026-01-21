#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "linked_list.h"

Node* head = NULL;


void func_BG(char **cmd){

  /*
  bg foo, your PMan will start the program foo in the background. That is,39
  the program foo will execute and PMan will also continue to execute and give the prompt to accept40
  more commands.
  */
  
  if (cmd[1] == NULL) {
    printf("No bg names provided, not starting any background processes.\n");
    return;
  }

  char **args = &cmd[1];

  pid_t pid = fork();

  if (pid == 0) {
    execvp(args[0], args);
    // func returns only if child execvp fails
    // CHECK FOR KNOWN VS UNKNOWN COMMANDS
    perror("execvp failed");
    exit(EXIT_FAILURE);

  } else if (pid > 0) {
    printf("Started bg process %d\n", pid);
    head = add_newNode(head, pid, cmd[1]);
  }
  else {
    perror("fork failed");
  }

  return;
}


void func_BGlist(char **cmd){
	printList(head);
}


void func_BGkill(char * str_pid){
	if (str_pid == NULL) {
		printf("Error: No PID provided\n");
		return;
	}
	
	pid_t pid = atoi(str_pid);
	
	if (PifExist(head, pid)) {
		if (kill(pid, SIGTERM) == 0) {
			printf("Process %d killed successfully\n", pid);
			// remove from linekd list
			head = deleteNode(head, pid);
		} else {
			perror("Failed to kill process");
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
			printf("Process %d stopped successfully\n", pid);
		} else {
			perror("Failed to stop process");
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
			printf("Process %d continued successfully\n", pid);
		} else {
			perror("Failed to continue process");
		}
	} else {
		printf("Process %d not found in background list\n", pid);
	}
}

void func_pstat(char * str_pid){
	if (str_pid == NULL) {
		printf("Error: No PID provided\n");
		return;
	}
	
	pid_t pid = atoi(str_pid);
	
	if (PifExist(head, pid)) {
    FILE *file;
    char line[256];
    char statpath[256];
    char statuspath[256];

    // setting variables to hold statistics
    char comm[256];
    char state;
    // long/unsignedlong variable because this is the types for these stats that the kernel exposes
    unsigned long utime, stime;
    long rss;
    unsigned long voluntary;
    unsigned long nonvoluntary;
    snprintf(statpath, sizeof(statpath), "/proc/%d/stat", pid);
    
    // fetching info found in /stat (comm, state, utime, stime, rss)
    file = fopen(statpath, "r");
    if (!file) {
      printf("Error: Process %d statistics not found.\n", pid);
      return;
    }
    fscanf(
      file,
      "%*d (%255[^)]) %c "
      "%*d %*d %*d %*d %*d "
      "%*u %*u %*u %*u "
      "%lu %lu %*d %*d %*d %*d %*d %*d "
      "%ld",
      comm,
      &state,
      &utime,
      &stime,
      &rss
    );
    fclose(file);
    
    // fetching info found in /status (ctxt_switch counts)
    snprintf(statuspath, sizeof(statuspath), "/proc/%d/status", pid);
    file = fopen(statuspath, "r");
    if (!file) {
      printf("Error: Process %d status statistics not found.\n", pid);
      return;
    }
    while (fgets(line, sizeof(line), file)) {
      if (sscanf(line, "voluntary_ctxt_switches: %lu", &voluntary) == 1) {
        continue;
      }
      if (sscanf(line, "nonvoluntary_ctxt_switches: %lu", &nonvoluntary) == 1) {
        continue;
      }
    }
		fclose(file);
    
    // printing formatted process statistics
    printf("<=== PID %d statistics ===>\n\n", pid);

    printf(
      "comm: %s\n"
      "state: %c\n"
      "utime: %lu\n"
      "stime: %lu\n"
      "rss: %ld\n"
      "voluntary context switch count: %lu\n"
      "nonvoluntary context switch count: %lu\n",
      comm, state, utime, stime, rss, voluntary, nonvoluntary);

  } else {
		printf("Process %d not found\n", pid);
	}
}

 
int main(){
    char user_input_str[50];
    while (true) {
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
        printf("Bye Bye \n");
        exit(0);
      } else {
        printf("Invalid input\n");
      }
    }

  return 0;
}

