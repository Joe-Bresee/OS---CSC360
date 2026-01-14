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