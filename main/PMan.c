// library imports
#include <stdlib.h>
#include <stdio.h>

char USER_NAME[] = "user";

void jsh_loop() {

    int status;

    do {
        printf("%c->", USER_NAME)
        line = jsh_read_line();
        status = jsh_execute(args);

    } while status;
}

#define JSH_RL_BUFSIZE=1024
char *jsh_read_line(void) {

    // allocating memory for reading input line
    int bufsize = JSH_RL_BUFSIZE;
    char *buf = (malloc(sizeof(char) * bufsize))
    
    if (!buf) {
        fprintf(stderr, "jsh: error allocating input buffer.\n")
        exit(EXIT_FAILURE);
    }
    
    // reading line operations
    int c;
    int position = 0;
    while(1) {
        // get char from stdin
        c = getchar();

        // handle line/file end symbols
        if (c == "EOF" || c == "\n") {
            buf[position] = "\0"; return buf;
        } else {
            buf[position] = c;
        }
        position++; 

        // alloc more buf
        if (position >= bufsize) {
            bufsize += JSH_RL_BUFSIZE;
            buf = realloc(buf, bufsize);
        }
        if (!buf) {
        fprintf(stderr, "jsh: error reallocating input buffer.\n");
        exit(EXIT_FAILURE);
    }
    }
    return buf;
}
int main {

    jsh_loop();

    return 0;
}

int main(){

	while(1){	
		cmd = readline("PMan: > ");
		
		/* parse the input cmd (e.g., with strtok)
		 */
		
		if (cmd_type == CMD_BG){
			bg_entry(argv);
		}
		else if(cmd_type == CMD_BGLIST){
			bglist_entry();
		}
		else if(cmd_type == CMD_BGKILL || cmd_type == CMD_BGSTOP || cmd_type == CMD_BGCONT){
			bgsig_entry(pid, cmd_type);
		}
		else if(cmd_type == CMD_PSTAT){
			pstat_entry(pid);
		}
		else {
			usage_pman();
		}
		check_zombieProcess();
	}
	return 0;
}

void bg_entry(char **argv){
	
	pid_t pid;
	pid = fork();
	if(pid == 0){
		if(execvp(argv[0], argv) < 0){
			perror("Error on execvp");
		}
		exit(EXIT_SUCCESS);
	}
	else if(pid > 0) {
		// store information of the background child process in your data structures
	}
	else {
		perror("fork failed");
		exit(EXIT_FAILURE);
	}
}

void check_zombieProcess(void){
	int status;
	int retVal = 0;
	
	while(1) {
		usleep(1000);
		if(headPnode == NULL){
			return ;
		}
		retVal = waitpid(-1, &status, WNOHANG);
		if(retVal > 0) {
			//remove the background process from your data structure
		}
		else if(retVal == 0){
			break;
		}
		else{
			perror("waitpid failed");
			exit(EXIT_FAILURE);
		}
	}
	return ;
}