/**
 Skeleton code of assignment 2 (For reference only)
	   
 The time calculation could be confusing, check the example of gettimeofday on tutorial for more detail.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

struct customer_info{ // use this struct to record the customer information read from customers.txt
    int user_id;
	int class_type;
	int service_time;
	int arrival_time;
};

/* global variables */

#define NQUEUE 2  // Define number of queues (business and economy)
#define NClerks 5  // Define number of clerks
#define TRUE 1     // Define TRUE
#define FALSE 0    // Define FALSE
#define IDLE TRUE     // Define idle status
#define MAX_CUSTOMERS 10000
 
struct timeval init_time; // use this variable to record the simulation start time; No need to use mutex_lock when reading this variable since the value would not be changed by thread once the initial time was set.
double overall_waiting_time; //A global variable to add up the overall waiting time for all customers, every customer add their own waiting time to this variable, mutex_lock is necessary.
struct customer_info business_queue[MAX_CUSTOMERS];
struct customer_info economy_queue[MAX_CUSTOMERS];
int business_queue_length = 0;
int economy_queue_length = 0;

int queue_status[NQUEUE]; // variable to record the status of a queue, the value could be idle (not using by any clerk) or the clerk id (1 ~ 4), indicating that the corresponding clerk is now signaling this queue.
int winner_selected[NQUEUE] = {FALSE, FALSE}; // variable to record if the first customer in a queue has been successfully selected and left the queue.

/* Other global variable may include: 
 1. condition_variables (and the corresponding mutex_lock) to represent each queue; 
 2. condition_variables to represent clerks
 3. others.. depend on your design
 */


void enqueue(struct customer_info *customer){

	// TODO: doublecheck correct naming of p_myInfo
	// TODO: create queue dsa's

	// business status case
	if (customer->class_type == 1) {
		business_queue[business_queue_length] = *customer;
		business_queue_length ++;
		printf("Customer %d entered the business queue", p_myInfo.user_id, len(b_queue));
		} else {
			economy_queue[economy_queue_length] = *customer;
			economy_queue_length ++;
		}
}

int main(int argc, char *argv[]) {

	if (argc < 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

 	char *input_file = argv[1];

	// initialize all the condition variable and thread lock will be used
	

	// Read customer information from txt file and store them in the structure you created
		
		// 1. Allocate memory(array, link list etc.) to store the customer information.
		int NCustomers;
		struct customer_info *customers;
		char line[256];
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


	//create clerk thread (optional)
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
	// destroy mutex & condition variable (optional)
	
	// calculate the average waiting time of all customers
	double average_wait = overall_waiting_time / NCustomers;
	printf("Average waiting time: %.2f\n", average_wait);
	
	// Free allocated memory
	free(customers);
	
	return 0;
}

// function entry for customer threads

void * customer_entry(void * cus_info){
	
	struct customer_info * p_myInfo = (struct info_node *) cus_info;
	
	// sleep arrival time of customer from program start
	usleep(p_myInfo->arrival_time);
	
	fprintf(stdout, "A customer arrives: customer ID %2d. \n", p_myInfo->user_id);
	
	/* Enqueue operation: get into either business queue or economy queue by using p_myInfo->class_type*/
	enqueue(p_myInfo);
	
	pthread_mutex_lock(/* mutexLock of selected queue */); 
	{
		fprintf(stdout, "A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", /*...*/);

		enQueue();
		queue_enter_time = getCurSystemTime();
		queue_length[cur_queue]++;
		
		while (TRUE) {
			pthread_cond_wait(/* cond_var of selected queue */, /* mutexLock of selected queue */);
			if (I_am_Head_of_the_Queue && !winner_selected[cur_queue]) {
				deQueue();
				queue_length[cur_queue]--;
				winner_selected[cur_queue] = TRUE; // update the winner_selected variable to indicate that the first customer has been selected from the queue
				break;
			}
		}
			
	}
	pthread_mutex_unlock(/*mutexLock of selected queue*/); //unlock mutex_lock such that other customers can enter into the queue
	
	/* Try to figure out which clerk awoken me, because you need to print the clerk Id information */
	usleep(10); // Add a usleep here to make sure that all the other waiting threads have already got back to call pthread_cond_wait. 10 us will not harm your simulation time.
	clerk_woke_me_up = queue_status[cur_queue];
	queue_status[cur_queue] = IDLE;
	
	/* get the current machine time; updates the overall_waiting_time*/
	
	fprintf(stdout, "A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", /*...*/);
	
	usleep(/* as long as the service time of this customer */);
	
	/* get the current machine time; */
	fprintf(stdout, "A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", /* ... */);\
	
	pthread_cond_signal(/* convar of the clerk signaled me */); // Notify the clerk that service is finished, it can serve another customer
	
	pthread_exit(NULL);
	
	return NULL;
}

// function entry for clerk threads
void *clerk_entry(void * clerkNum){
	
	while(TRUE){
		
		/* selected_queue_ID = Select the queue based on the priority and current customers number */
		
		pthread_mutex_lock(/* mutexLock of the selected queue */);
		
		queue_status[selected_queue_ID] = clerkID; // The current clerk (clerkID) is signaling this queue
		
		pthread_cond_broadcast(/* cond_var of the selected queue */); // Awake the customer (the one enter into the queue first) from the selected queue
		
		winner_selected[selected_queue_ID] = FALSE; // set the initial value as the customer has not selected from the queue.
		
		pthread_mutex_unlock(/* mutexLock of the selected queue */);
		
		pthread_cond_wait(/* convar of the current clerk */); // wait for the customer to finish its service
	}
	
	pthread_exit(NULL);
	
	return NULL;
}