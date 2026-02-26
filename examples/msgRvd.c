#include <stdio.h>
#include <sys/msg.h>

int main() {
  int msqid = 0;
  struct message {
    long type;
    char text[20];
  } msg;
  long msgtyp = 0;

  printf("Enter the message Queue ID:");   
  scanf("%d",&msqid);
  printf("Enter the message type:");
  scanf("%ld",&msgtyp);


  msgrcv(msqid, (void *) &msg, sizeof(msg.text), msgtyp, MSG_NOERROR | IPC_NOWAIT);
  printf("%s \n", msg.text);

  return 0;
}

