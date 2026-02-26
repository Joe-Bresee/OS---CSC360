#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>

#define SHMSZ     27

int main()
{
    char c;
    int shmid;
    key_t key;
    char *shm, *s;
    
    /*
     * We'll name our shared memory segment
     * "5678".
     * Be careful about the key. For safety, it is better to use ftok() 
     */
    key = 5678;

    /*
     * Create the segment.
     */
    if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0777)) < 0) {
        perror("shmget");
        return(1);
    }

    /*
     * Now we attach the segment to our data space.
     */
    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("shmat");
        return(1);
    }

    /*
     * Now put some things into the memory for the
     * other process to read.
     */
    s = shm;

    for (c = 'a'; c <= 'z'; c++)
        *s++ = c;
    *s = '\0';

    /*
     * Finally, we wait until the other process 
     * changes the first character of our memory
     * to '*', indicating that it has read what 
     * we put there.
     */
    while (*shm != '*')
        sleep(1);

   /* 
    * The server needs to detach or destroy the shared memory
   */

    if (shmdt(shm)==-1)
       perror("shmop: shmdt failed");
/*
    if (shmctl(shmid,IPC_RMID,NULL) ==-1)
       perror("shmop: shmctl failer");
*/
    return(0);
}


