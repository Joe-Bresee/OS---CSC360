#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "linked_list.h"

 
Node * add_newNode(Node* head, pid_t new_pid, char * new_path){
	// Allocate memory for new node on heap
	Node *newNode = malloc(sizeof(Node));
	if (newNode == NULL) {
		return head; // malloc failed
	}
	
	// Set the new node's data
	newNode->pid = new_pid;
	newNode->path = malloc(strlen(new_path) + 1);
	if (newNode->path == NULL) {
		free(newNode);
		return head; // malloc failed
	}
	strcpy(newNode->path, new_path);
	
	newNode->next = head;
	
	return newNode;
}


Node * deleteNode(Node* head, pid_t pid){
	
	if(head == NULL) {
		printf("List is already empty\n");
		return NULL;
	}

	if(head->pid == pid) {
		Node* temp = head;
		head = head->next;
		free(temp->path);
		free(temp);        
		return head;
	}

	Node* current = head;
	while(current->next != NULL) {
		if (current->next->pid == pid) {
			Node* nodeToDelete = current->next;
			current->next = nodeToDelete->next;
			free(nodeToDelete->path);            
			free(nodeToDelete);                  
			return head;                         
		}
		current = current->next;
	}
	printf("PID %d not found in list\n", pid);
	return head;
}

void printList(Node *node){
	if (node == NULL) {
		printf("List is empty\n");
		return;
	}
	
	while(node != NULL) {
		printf("pid: %d, path: %s\n", node->pid, node->path);
		node = node->next;
	}
	return;
}

int PifExist(Node *node, pid_t pid){
	while (node != NULL) {
		if (node->pid == pid) {
			return 1;
		}
		node = node->next;
	}
	return 0;
}