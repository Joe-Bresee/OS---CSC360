#include <string.h>
#include <sys/msg.h> 

int main() {
  int msqid = 0;
  struct message {
    long type;
    char text[20];
  } msg;

  key_t key;
  if ((key = ftok("/home/.", 'B')) == -1) {
        perror("ftok");
        return 1;
  }
  if ((msqid = msgget(key, 0666 | IPC_CREAT)) == -1) {
        perror("msgget");
        return 1;
  }
  msg.type = 1;
  strcpy(msg.text, "This is message 1");
  msgsnd(msqid, (void *) &msg, sizeof(msg.text), IPC_NOWAIT);
  strcpy(msg.text, "This is message 2");
  msgsnd(msqid, (void *) &msg, sizeof(msg.text), IPC_NOWAIT);
  return 0;
}

