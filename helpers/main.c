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
	printf("=== Command Structure Debug ===\n");
	printf("cmd[0] (command): %s\n", cmd[0]);
	
	// Print all arguments
	int i = 0;
	while(cmd[i] != NULL) {
		printf("cmd[%d]: %s\n", i, cmd[i]);
		i++;
	}
	printf("Total arguments: %d\n", i);
	printf("==============================\n");
}


void func_BGlist(char **cmd){
	//Your code here;
}


void func_BGkill(char * str_pid){
	//Your code here
}


void func_BGstop(char * str_pid){
	//Your code here
}


void func_BGstart(char * str_pid){
	//Your code here
}


void func_pstat(char * str_pid){
	//Your code here
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

