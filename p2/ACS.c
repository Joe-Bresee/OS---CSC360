#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#define NQUEUE 2  // Define number of queues (business and economy)
#define NClerks 5  // Define number of clerks
#define TRUE 1     // Define TRUE
#define FALSE 0    // Define FALSE
#define IDLE TRUE     // Define idle status
#define MAX_CUSTOMERS 10000

typedef struct customer_info{ // use this struct to record the customer information read from customers.txt
    int user_id;
	int class_type;
	int service_time;
	int arrival_time;

	int selected;
	int assigned_clerk;
	double queue_enter_time;

	pthread_cond_t cond;
} customer_t;

customer_t *customers;

customer_t *business_queue[MAX_CUSTOMERS];
customer_t *economy_queue[MAX_CUSTOMERS];
int queue_length[NQUEUE] = {0, 0};

int NCustomers;
int finished_customers = 0;

double total_wait_all = 0;
double total_wait_business = 0;
double total_wait_economy = 0;

int count_business = 0;
int count_economy = 0;

void *clerk_entry(void *clerkNum);
void *customer_entry(void *cus_info);
double curtimeofdaydouble();
void enqueue(struct customer_info *customer);
customer_t *dequeue(int queue_id);

struct timeval init_time; // use this variable to record the simulation start time; No need to use mutex_lock when reading this variable since the value would not be changed by thread once the initial time was set.

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;


double get_time() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec - init_time.tv_sec)
           + (now.tv_usec - init_time.tv_usec)/1000000.0;
}

void enqueue(struct customer_info *customer){

	// business status case
	if (customer->class_type == 1) {
		business_queue[queue_length[1]] = customer;
		queue_length[1]++;
		printf("Customer %d entered the business queue\n", customer->user_id);
		} else {
			economy_queue[queue_length[0]] = customer;
			queue_length[0]++;
			printf("Customer %d entered the economy queue\n", customer->user_id);
		}
}

customer_t* dequeue(int queue_id) {
    customer_t *customer;
    if (queue_id == 1) {
        customer = business_queue[0];
        for (int i = 0; i < queue_length[1]-1; i++)
            business_queue[i] = business_queue[i+1];
        queue_length[1]--;
    } else {
        customer = economy_queue[0];
        for (int i = 0; i < queue_length[0]-1; i++)
            economy_queue[i] = economy_queue[i+1];
        queue_length[0]--;
    }
    return customer;
}

int main(int argc, char *argv[]) {

	// recording simulation start time
	gettimeofday(&init_time, NULL);

	if (argc < 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

 	char *input_file = argv[1];	

	// Read customer information from txt file and store them in the structure you created
		
		// 1. Allocate memory(array, link list etc.) to store the customer information.
		struct customer_info *customers;
		int i = 0;

		FILE *file = fopen(input_file, "r");
		if (!file) { 
			return -1;
		}
		
		if (fscanf(file, "%d", &NCustomers) != 1) {
			fclose(file);
			return -1;
		}
		
		// Allocate memory for all customers
		customers = malloc(NCustomers * sizeof(customer_t));
		if (!customers) {
			fclose(file);
			return -1;
		}


		// 2. File operation: read each customer line with format id:class,arrival_time,service_time
		for (i = 0; i < NCustomers; i++) {
			fscanf(file, "%d:%d,%d,%d", 
				&customers[i].user_id,
				&customers[i].class_type, 
				&customers[i].arrival_time,
				&customers[i].service_time);
				
			customers[i].selected = 0;
			pthread_cond_init(&customers[i].cond, NULL);
			
			// Input validation for customer data
			if (customers[i].user_id <= 0) {
				fprintf(stderr, "Error: Customer %d has invalid user ID (%d). Must be positive.\n", 
					   i + 1, customers[i].user_id);
				free(customers);
				fclose(file);
				return -1;
			}
			
			if (customers[i].class_type != 0 && customers[i].class_type != 1) {
				fprintf(stderr, "Error: Customer %d has invalid class type (%d). Must be 0 (economy) or 1 (business).\n", 
					   i + 1, customers[i].class_type);
				free(customers);
				fclose(file);
				return -1;
			}
			
			if (customers[i].arrival_time < 0) {
				fprintf(stderr, "Error: Customer %d has negative arrival time (%d). Must be non-negative.\n", 
					   i + 1, customers[i].arrival_time);
				free(customers);
				fclose(file);
				return -1;
			}
			
			if (customers[i].service_time <= 0) {
				fprintf(stderr, "Error: Customer %d has invalid service time (%d). Must be positive.\n", 
					   i + 1, customers[i].service_time);
				free(customers);
				fclose(file);
				return -1;
			}
			
			printf("Loaded customer %d: class=%s, arrival=%d, service=%d\n", 
				   customers[i].user_id,
				   (customers[i].class_type == 1) ? "business" : "economy",
				   customers[i].arrival_time,
				   customers[i].service_time);
		}

		fclose(file);
		printf("Successfully loaded %d customers\n", NCustomers);


	//create clerk thread
	pthread_t clerkId[NClerks];
	int clerk_info[NClerks];
	for(i = 0; i < NClerks; i++){ // number of clerks
		clerk_info[i] = i + 1; // clerk IDs 1-5
		pthread_create(&clerkId[i], NULL, clerk_entry, &clerk_info[i]); // clerk_info: passing the clerk information (e.g., clerk ID) to clerk thread
	}
	
	//create customer thread
	pthread_t customId[NCustomers];
	for(i = 0; i < NCustomers; i++){ // number of customers
		pthread_create(&customId[i], NULL, customer_entry, &customers[i]); //customers: passing the customer information to customer thread
	}
	// wait for all customer threads to terminate
	for(i = 0; i < NCustomers; i++){
		pthread_join(customId[i], NULL);
	}
	// stop clerks
	for (i = 0; i < NClerks; i++) {
    pthread_join(clerkId[i], NULL);
	}

	printf("The average waiting time for all customers in the system is: %.2f seconds.\n",
           total_wait_all / NCustomers);

    printf("The average waiting time for all business-class customers is: %.2f seconds.\n",
           count_business ? total_wait_business / count_business : 0.0);

    printf("The average waiting time for all economy-class customers is: %.2f seconds.\n",
           count_economy ? total_wait_economy / count_economy : 0.0);

    free(customers);

    for (int i = 0; i < NCustomers; i++) {
        pthread_cond_destroy(&customers[i].cond);
    }

    return 0;
}

// function entry for customer threads

void * customer_entry(void * cus_info){
	
	customer_t * p_myInfo = (customer_t *) cus_info;
	
	// sleep arrival time of customer from program start
	usleep(p_myInfo->arrival_time * 100000);
	
	fprintf(stdout, "Customer with ID %2d arrives at time %d\n", p_myInfo->user_id, p_myInfo->arrival_time);
	
	pthread_mutex_lock(&queue_mutex);
	
	//enqueue this customer
	p_myInfo->queue_enter_time = get_time();
	enqueue(p_myInfo);

	fprintf(stdout, "A customer enters a queue: the queue ID is %1d, and the length of the queue is %2d. \n", p_myInfo->class_type, queue_length[p_myInfo->class_type]);

	pthread_cond_signal(&queue_not_empty);

	while (!p_myInfo->selected)
		pthread_cond_wait(&p_myInfo->cond, &queue_mutex);

	pthread_mutex_unlock(&queue_mutex);

	return NULL;
}

// function entry for clerk threads
void *clerk_entry(void * clerkNum){
    int clerkID = *(int*)clerkNum;
    
    while(TRUE){
        pthread_mutex_lock(&queue_mutex);

        while (queue_length[1] == 0 && queue_length[0] == 0) {
            if (finished_customers == NCustomers) {
                pthread_mutex_unlock(&queue_mutex);
                return NULL;
            }

            fprintf(stdout, "clerk %d is going idle\n", clerkID);
            pthread_cond_wait(&queue_not_empty, &queue_mutex);
        }

		// dequeue
        int selected_queue_ID = (queue_length[1] > 0) ? 1 : 0;
        customer_t *customer = dequeue(selected_queue_ID);
        
		// calculate waiting time
		double start_time_service = get_time();
		double wait_time = start_time_service - customer->queue_enter_time;

		total_wait_all += wait_time;

		if (customer->class_type == 1) {
			total_wait_business += wait_time;
			count_business++;
		} else {
			total_wait_economy += wait_time;
			count_economy++;
		}
        
		// mark as seleected and sign clerk
		customer->selected = 1;
		customer->assigned_clerk = clerkID;

		fprintf(stdout, "A clerk starts serving a customer: start time %.2f, customer ID %2d, the clerk ID %1d.\n", start_time_service, customer->user_id, clerkID);
        
		pthread_cond_signal(&customer->cond);
        pthread_mutex_unlock(&queue_mutex);


        // service customer
		usleep(customer->service_time * 100000);

		double end_time_service = get_time();

		printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d.\n", end_time_service, customer->user_id, clerkID);

		pthread_mutex_lock(&queue_mutex);

		finished_customers++;

		// signal other clerks finished scheduling all customers
		if (finished_customers == NCustomers) {
			pthread_cond_broadcast(&queue_not_empty);
		}
		pthread_mutex_unlock(&queue_mutex);
    }
}