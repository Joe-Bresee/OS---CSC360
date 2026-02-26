// TODO: add timestamp to each print statement

/**
 Skeleton code of assignment 2 (For reference only)
	   
 The time calculation could be confusing, check the example of gettimeofday on tutorial for more detail.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

double timeval_to_double(struct timeval tv) {
	return tv.tv_sec + tv.tv_usec / 1000000.0;
}

struct customer_info{ // use this struct to record the customer information read from customers.txt
    int user_id;
	int class_type;
	int service_time;
	int arrival_time;
};

void *clerk_entry(void *clerkNum);
void *customer_entry(void *cus_info);
double curtimeofdaydouble();
void enqueue(struct customer_info *customer);
struct customer_info dequeue(int queue_id);

/* global variables */

#define NQUEUE 2  // Define number of queues (business and economy)
#define NClerks 5  // Define number of clerks
#define TRUE 1     // Define TRUE
#define FALSE 0    // Define FALSE
#define IDLE TRUE     // Define idle status
#define MAX_CUSTOMERS 10000

// customer done flag
int all_customers_done = 0;
 
struct timeval init_time; // use this variable to record the simulation start time; No need to use mutex_lock when reading this variable since the value would not be changed by thread once the initial time was set.
double overall_waiting_time; //A global variable to add up the overall waiting time for all customers, every customer add their own waiting time to this variable, mutex_lock is necessary.

// queue dsa globals
struct customer_info business_queue[MAX_CUSTOMERS];
struct customer_info economy_queue[MAX_CUSTOMERS];
int queue_length[NQUEUE] = {0, 0};
int queue_status[NQUEUE] = {IDLE, IDLE}; // variable to record the status of a queue, the value could be idle (not using by any clerk) or the clerk id (1 ~ 4), indicating that the corresponding clerk is now signaling this queue.
int winner_selected[NQUEUE] = {FALSE, FALSE}; // variable to record if the first customer in a queue has been successfully selected and left the queue.

/* Other global variable may include: 
 1. condition_variables (and the corresponding mutex_lock) to represent each queue; 
 2. condition_variables to represent clerks
 3. others.. depend on your design
 */

//  mutex protecting both queues
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
// overall_waiting_time mutex
pthread_mutex_t waiting_time_mutex = PTHREAD_MUTEX_INITIALIZER;

// condition variable to wake up idle clerks when customer joins queue
pthread_cond_t queue_convar = PTHREAD_COND_INITIALIZER;
// convar per clerk so customer can signal the right clerk when finished servicing.
pthread_cond_t clerk_convar[NClerks];

void enqueue(struct customer_info *customer){

	// TODO: doublecheck correct naming of p_myInfo
	// TODO: create queue dsa's

	// business status case
	if (customer->class_type == 1) {
		business_queue[queue_length[1]] = *customer;
		queue_length[1]++;
		printf("Customer %d entered the business queue\n", customer->user_id);
		} else {
			economy_queue[queue_length[0]] = *customer;
			queue_length[0]++;
			printf("Customer %d entered the economy queue\n", customer->user_id);
		}
}

struct customer_info dequeue(int queue_id) {
	// dequeue customer from front of queue based on the queue id
	struct customer_info customer = (queue_id == 1) ? business_queue[0] : economy_queue[0];

	// shift each customer forward in queue because first customer is gone.
	if (queue_id == 1) {
		for (int i = 0; i < queue_length[1] - 1; i++) {
			business_queue[i] = business_queue[i + 1];
		}
	queue_length[1]--;
	} else {
		for (int i = 0; i < queue_length[0] - 1; i++) {
			economy_queue[i] = economy_queue[i + 1];
		}
	queue_length[0]--;
	}

	return customer;
}

double curtimeofdaydouble() {
	struct timeval curtime;
	gettimeofday(&curtime, NULL);
	return timeval_to_double(curtime) - timeval_to_double(init_time);
}

int main(int argc, char *argv[]) {

	// recording simulation start time
	gettimeofday(&init_time, NULL);

	if (argc < 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

 	char *input_file = argv[1];

	// initialize all the condition variable and thread lock will be used
	for (int i = 0; i < NClerks; i++) {
		pthread_cond_init(&clerk_convar[i], NULL);
	}
	

	// Read customer information from txt file and store them in the structure you created
		
		// 1. Allocate memory(array, link list etc.) to store the customer information.
		int NCustomers;
		struct customer_info *customers;
		int i = 0;
		
		// TODO: input checking for negative arriveal times etc.
		FILE *file = fopen(input_file, "r");
		if (!file) { 
			return -1;
		}
		
		if (fscanf(file, "%d", &NCustomers) != 1) {
			fclose(file);
			return -1;
		}
		
		// Allocate memory for all customers
		customers = malloc(NCustomers * sizeof(struct customer_info));
		if (!customers) {
			fclose(file);
			return -1;
		}


		// 2. File operation: read each customer line with format id:class,arrival_time,service_time
		for (i = 0; i < NCustomers; i++) {
			if (fscanf(file, "%d:%d,%d,%d", 
					   &customers[i].user_id,
					   &customers[i].class_type, 
					   &customers[i].arrival_time,
					   &customers[i].service_time) != 4) {
				fprintf(stderr, "Error parsing customer %d\n", i + 1);
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
		pthread_create(&clerkId[i], NULL, clerk_entry, (void *)&clerk_info[i]); // clerk_info: passing the clerk information (e.g., clerk ID) to clerk thread
	}
	
	//create customer thread
	pthread_t customId[NCustomers];
	for(i = 0; i < NCustomers; i++){ // number of customers
		pthread_create(&customId[i], NULL, customer_entry, (void *)&customers[i]); //customers: passing the customer information to customer thread
	}
	// wait for all customer threads to terminate
	for(i = 0; i < NCustomers; i++){
		pthread_join(customId[i], NULL);
	}

	// unhang the clerks
	all_customers_done = 1;
	
	// calculate the average waiting time of all customers
	double average_wait = overall_waiting_time / NCustomers;
	printf("Average waiting time: %.2f\n", average_wait);
	
	// Free allocated memory
	free(customers);

	// explicitly destroying mutexs and convars before returning main
	pthread_mutex_destroy(&queue_mutex);
	pthread_mutex_destroy(&waiting_time_mutex);
	pthread_cond_destroy(&queue_convar);
	for (int i = 0; i < NClerks; i++) {
		pthread_cond_destroy(&clerk_convar[i]);
	}
	
	return 0;
}

// function entry for customer threads

void * customer_entry(void * cus_info){
	
	struct customer_info * p_myInfo = (struct customer_info *) cus_info;
	
	// sleep arrival time of customer from program start
	usleep(p_myInfo->arrival_time * 100000);
	
	fprintf(stdout, "Customer with ID %2d arrives at %d\n", p_myInfo->user_id, p_myInfo->arrival_time);
	
	// set enter time for later calculation of waiting time
	double queue_enter_time = curtimeofdaydouble();
	
	// define before mutex_lock to keep value in needed scope
	int cur_queue = p_myInfo->class_type;

	pthread_mutex_lock(&queue_mutex); 
	{
		enqueue(p_myInfo);
		// TODO: refactor queue setup so easy to access lenth and stuff
		fprintf(stdout, "A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", p_myInfo->class_type, queue_length[cur_queue]);

		while (TRUE) {
			// wait for clerk to take me
			pthread_cond_wait(&queue_convar, &queue_mutex);
			int head_id = (cur_queue == 1) ? business_queue[0].user_id : economy_queue[0].user_id;
			if (p_myInfo->user_id == head_id && !winner_selected[cur_queue]) {
				dequeue(p_myInfo->class_type);
				winner_selected[cur_queue] = TRUE; // update the winner_selected variable to indicate that the first customer has been selected from the queue
				break;
			}
		}
			
	}
	pthread_mutex_unlock(&queue_mutex); //unlock mutex_lock such that other customers can enter into the queue
	
	/* Try to figure out which clerk awoken me, because you need to print the clerk Id information */
	usleep(10); // Add a usleep here to make sure that all the other waiting threads have already got back to call pthread_cond_wait. 10 us will not harm your simulation time.
	int clerk_woke_me_up = queue_status[cur_queue];
	queue_status[cur_queue] = IDLE;
	
	/* get the current machine time; updates the overall_waiting_time*/
	double svc_start_time = curtimeofdaydouble();
	double cur_waiting_time = svc_start_time - queue_enter_time;
	pthread_mutex_lock(&waiting_time_mutex);
	overall_waiting_time += cur_waiting_time;
	pthread_mutex_unlock(&waiting_time_mutex);

	fprintf(stdout, "A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", svc_start_time, p_myInfo->user_id, clerk_woke_me_up);
	
	usleep(p_myInfo->service_time * 100000);
	
	fprintf(stdout, "A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", curtimeofdaydouble(), p_myInfo->user_id, clerk_woke_me_up);
	
	pthread_cond_signal(&clerk_convar[clerk_woke_me_up - 1]); // Notify the clerk that service is finished, it can serve another customer
	
	pthread_exit(NULL);
	
	return NULL;
}

// function entry for clerk threads
void *clerk_entry(void * clerkNum){

	// dereference clerkNum pointer passed to clerk_entry to access clerk_info[i] int
	int clerkID = *(int*)clerkNum;
	
	while(TRUE){

		pthread_mutex_lock(&queue_mutex);

		/* selected_queue_ID = Select the queue based on the priority and current customers number */
		int selected_queue_ID;
		if (queue_length[1] > 0) {
			selected_queue_ID = 1;
		} else if (queue_length[0] > 0) {
			selected_queue_ID = 0;
		} else {
			if (all_customers_done) {
				pthread_mutex_unlock(&queue_mutex);
				pthread_exit(NULL);
			}
			fprintf(stdout, "clerk %d is going idle, as no customers are in either queue.", clerkID);
			pthread_cond_wait(&queue_convar, &queue_mutex);
			pthread_mutex_unlock(&queue_mutex);
			continue;
		}
		
		queue_status[selected_queue_ID] = clerkID; // The current clerk (clerkID) is signaling this queue
		
		pthread_cond_broadcast(&queue_convar); // Awake the customer (the one enter into the queue first) from the selected queue
		
		winner_selected[selected_queue_ID] = FALSE; // set the initial value as the customer has not selected from the queue.
		
		pthread_mutex_unlock(&queue_mutex);
		
		pthread_cond_wait(&clerk_convar[clerkID - 1], &queue_mutex); // wait for the customer to finish its service
	}
	
	pthread_exit(NULL);
	
	return NULL;
}