#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#define NQUEUE 2
#define NCLERKS 5
#define MAX_CUSTOMERS 10000

typedef struct customer_info {
    int user_id;
    int class_type;        // 0 = economy, 1 = business
    int arrival_time;
    int service_time;

    int selected;
    int assigned_clerk;

    double queue_enter_time;

    pthread_cond_t cond;
} customer_t;

/* ================= GLOBALS ================= */

customer_t *customers;

customer_t *business_queue[MAX_CUSTOMERS];
customer_t *economy_queue[MAX_CUSTOMERS];

int queue_length[2] = {0,0};

int total_customers;
int finished_customers = 0;

double total_wait_all = 0;
double total_wait_business = 0;
double total_wait_economy = 0;

int count_business = 0;
int count_economy = 0;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;

struct timeval start_time;

/* ================= TIME ================= */

double get_time() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec - start_time.tv_sec)
           + (now.tv_usec - start_time.tv_usec)/1000000.0;
}

/* ================= QUEUE OPS ================= */

void enqueue(customer_t *c) {
    if (c->class_type == 1) {
        business_queue[queue_length[1]++] = c;
    } else {
        economy_queue[queue_length[0]++] = c;
    }
}

customer_t* dequeue(int qid) {
    customer_t *c;
    if (qid == 1) {
        c = business_queue[0];
        for (int i = 0; i < queue_length[1]-1; i++)
            business_queue[i] = business_queue[i+1];
        queue_length[1]--;
    } else {
        c = economy_queue[0];
        for (int i = 0; i < queue_length[0]-1; i++)
            economy_queue[i] = economy_queue[i+1];
        queue_length[0]--;
    }
    return c;
}

/* ================= CUSTOMER ================= */

void* customer_entry(void *arg) {
    customer_t *c = (customer_t*)arg;

    usleep(c->arrival_time * 100000);

    printf("A customer arrives: customer ID %2d.\n", c->user_id);

    pthread_mutex_lock(&queue_mutex);

    c->queue_enter_time = get_time();

    enqueue(c);

    printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d.\n",
           c->class_type,
           queue_length[c->class_type]);

    pthread_cond_signal(&queue_not_empty);

    while (!c->selected)
        pthread_cond_wait(&c->cond, &queue_mutex);

    pthread_mutex_unlock(&queue_mutex);

    return NULL;
}

/* ================= CLERK ================= */

void* clerk_entry(void *arg) {
    int clerk_id = *(int*)arg;

    while (1) {

        pthread_mutex_lock(&queue_mutex);

        while (queue_length[0] == 0 && queue_length[1] == 0) {

            if (finished_customers == total_customers) {
                pthread_mutex_unlock(&queue_mutex);
                return NULL;
            }

            pthread_cond_wait(&queue_not_empty, &queue_mutex);
        }

        int qid = (queue_length[1] > 0) ? 1 : 0;

        customer_t *c = dequeue(qid);

        double start_time_service = get_time();
        double wait_time = start_time_service - c->queue_enter_time;

        total_wait_all += wait_time;

        if (c->class_type == 1) {
            total_wait_business += wait_time;
            count_business++;
        } else {
            total_wait_economy += wait_time;
            count_economy++;
        }

        c->selected = 1;
        c->assigned_clerk = clerk_id;

        printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d.\n",
               start_time_service, c->user_id, clerk_id);

        pthread_cond_signal(&c->cond);

        pthread_mutex_unlock(&queue_mutex);

        /* ----------- SERVICE HAPPENS HERE ----------- */
        usleep(c->service_time * 100000);

        double end_time_service = get_time();

        printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d.\n",
               end_time_service, c->user_id, clerk_id);

        pthread_mutex_lock(&queue_mutex);

        finished_customers++;

        if (finished_customers == total_customers) {
            pthread_cond_broadcast(&queue_not_empty);
        }

        pthread_mutex_unlock(&queue_mutex);
    }
}

/* ================= MAIN ================= */

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s input_file\n", argv[0]);
        return 1;
    }

    gettimeofday(&start_time, NULL);

    FILE *file = fopen(argv[1], "r");
    if (!file) return 1;

    fscanf(file, "%d", &total_customers);

    customers = malloc(sizeof(customer_t) * total_customers);

    for (int i = 0; i < total_customers; i++) {

        fscanf(file, "%d:%d,%d,%d",
               &customers[i].user_id,
               &customers[i].class_type,
               &customers[i].arrival_time,
               &customers[i].service_time);

        customers[i].selected = 0;
        pthread_cond_init(&customers[i].cond, NULL);
    }

    fclose(file);

    pthread_t clerks[NCLERKS];
    pthread_t cust_threads[total_customers];

    int clerk_ids[NCLERKS];

    for (int i = 0; i < NCLERKS; i++) {
        clerk_ids[i] = i+1;
        pthread_create(&clerks[i], NULL, clerk_entry, &clerk_ids[i]);
    }

    for (int i = 0; i < total_customers; i++)
        pthread_create(&cust_threads[i], NULL, customer_entry, &customers[i]);

    for (int i = 0; i < total_customers; i++)
        pthread_join(cust_threads[i], NULL);

    for (int i = 0; i < NCLERKS; i++)
        pthread_join(clerks[i], NULL);

    printf("The average waiting time for all customers in the system is: %.2f seconds.\n",
           total_wait_all / total_customers);

    printf("The average waiting time for all business-class customers is: %.2f seconds.\n",
           count_business ? total_wait_business / count_business : 0.0);

    printf("The average waiting time for all economy-class customers is: %.2f seconds.\n",
           count_economy ? total_wait_economy / count_economy : 0.0);

    free(customers);

    for (int i = 0; i < total_customers; i++) {
        pthread_cond_destroy(&customers[i].cond);
    }

    return 0;
}