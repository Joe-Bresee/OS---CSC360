#include <stdio.h>
#include <unistd.h>
#include <math.h>

int main() {
    double sum = 0.0;

    // CPU-heavy loop to force user-mode time
    for (long i = 0; i < 100000000000; i++) {
        sum += sin(i) + cos(i); // floating-point math keeps CPU busy
    }

    // Keep process alive to monitor stats
    printf("PID %d running. Loop done. Now sleeping...\n", getpid());
    while(1) {
        usleep(100000); // sleep 0.1s in small chunks
    }

    return 0;
}