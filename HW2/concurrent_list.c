#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

//helper functoins
node* create_node(int value);
void delete_node(node* node);


struct node {//struct node
	int value;//node value
	node* next;//pointer to the next node in the list
	node* prev;//pointer to the previous node in the list
	pthread_mutex_t mut;//each node in the list has its own mutex
};



struct list {//struct list
	node* head;//pointer to the node that start the list
	pthread_mutex_t mut;//the list has its own lock so it will not be possible to read unfinished written data
};

void delete_node(node* node) {//helper function #1 , take pointer to a node, destroy its mutex and free the node 
	pthread_mutex_destroy(&(node->mut));
	free(node);
}

node* create_node(int value) {//helper function #2, take an integer value, create a node that its prev & next are null, and initialize its mutex as an error checking mutex
	node* nod = (node*)malloc(sizeof(node));//allocating new node
	if (!nod) {//if not enough memory
		printf("Error! not enough memory");
		exit(1);
	}
	nod->value = value;//setting the node value to value
	//setting the node prev & next to null
	nod->next = NULL;
	nod->prev = NULL;
	//initializing the mutex of the node as an error checking mutex 
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
	pthread_mutex_init(&(nod->mut), &attr);
	pthread_mutexattr_destroy(&attr);
	return nod;
}

void print_node(node* node) {
	// if there is a node, print its value with whitespace after
	if (node) {
		printf("%d ", node->value);
	}
}

list* create_list() {//function create_list, allocating new list with null head, and initializing its mutex as an error checking mutex
	list* lis = (list*)malloc(sizeof(list));//allocating new list
	if (!lis) {//if not enough memory
		printf("ERROR! not enough memory");
		exit(1);
	}
	lis->head = NULL;//setting list head to null
	//initializing list mutex as an error checking mutex
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
	pthread_mutex_init(&(lis->mut), &attr);
	pthread_mutexattr_destroy(&attr);
	return lis;
}

void delete_list(list* list) {//function delete_list
	if (!list) {//if no list, we don't need to do anyting
		return;
	}
	//getting the list head
	pthread_mutex_lock(&(list->mut));
	node* head = list->head;
	pthread_mutex_unlock(&(list->mut));
	if (!head) {//if there is no head, just destroy the list mutex and free the list
		pthread_mutex_destroy(&(list->mut));
		free(list);
		return;
	}
	//otherwise
	//get head->next 
	pthread_mutex_lock(&(head->mut));
	node* next = head->next;
	pthread_mutex_unlock(&(head->mut));
	while (head->next) {//while there is next to the list
		//lock both head & head->next mutexes
		pthread_mutex_lock(&(head->mut));
		pthread_mutex_lock(&(head->next->mut));
		next = head->next;//set the "next" variable as head->next
		//unlock both head & head->next mutexes
		pthread_mutex_unlock(&(next->mut));
		pthread_mutex_unlock(&(head->mut));
		//delete the list current head
		delete_node(head);
		//set head to its next
		head = next;
	}
	delete_node(head);//delete the last node on the list
	pthread_mutex_destroy(&(list->mut));//destroy the list mutex and free it
	free(list);
}

void insert_value(list* list, int value) {//function insert_value, take list pointer and integer value, and insert the value into the list such that the list will be remain sorted
	if (!list) {//even Gadi Landau don't know how to insert a value into non-existing list
		printf("you need to first initialize a list with create_list\n");
		return;
	}

	pthread_mutex_lock(&(list->mut));//we will first lock its mutex to prevent from any other thread to write to/read from the list head until we will finish writing there.
	if (!(list->head)) {//if the list is currently empty.
		list->head = create_node(value);//we will create a new node with the value "value" and assign it to the list head
		pthread_mutex_unlock(&(list->mut));//after all of that, we will relese the list mutex

	}
	else {//if the list isn't empty
		node* head = list->head;//we will get the list head
		pthread_mutex_unlock(&(list->mut));//after all of that, we will relese the list mutex
		pthread_mutex_lock(&(head->mut));//now we will lock the head mutex
		while ((head->next) && (head->next->value <= value)) {//while we have where to continue, and the next element isn't bigger than our value, run on the list with hand over hand locking
			pthread_mutex_lock(&(head->next->mut));//lock the next element on the list
			head = head->next;//assign head->next to be the new head
			pthread_mutex_unlock(&(head->prev->mut));//unlock the mutex of the head from the beginning of the loop
		}
		node* n = create_node(value);//create new node with the value "value"
		if ((!head->next) && (n->value >= head->value)) {//if we got to the end of the list
			//attach the new node to the end of the list
			head->next = n;
			n->prev = head;
			pthread_mutex_unlock(&(head->mut));//unlock the last mutex that wasn't unlocked yet
		}
		else if ((!head->prev) && (n->value < head->value)) {//if "value" should be inserted to the beginning of the list
			//attach the new node to the beginning of the list
			head->prev = n;
			n->next = head;
			pthread_mutex_unlock(&(head->mut));//unlock the last mutex that was locked
		}
		else {//if "value" should be inserted somewhere in the middle of the list(between head and head->next)
			pthread_mutex_lock(&(head->next->mut));//lock head->next mutex
			//insert the new node between head and head->next
			head->next->prev = n;
			n->next = head->next;
			n->prev = head;
			head->next = n;
			//unlock the mutexes of head and n->next(the mutex we've locked at the beginning of the else statment)
			pthread_mutex_unlock(&(n->next->mut));
			pthread_mutex_unlock(&(head->mut));
		}
		while (head->prev) {//now find the new list head
			head = head->prev;
		}

		pthread_mutex_lock(&(list->mut));//now lock the list mutex so that no other thread will be able to read from/write to the list head while we writing there
		list->head = head;//updating the list head with the new list head(in case that there was no change in the list head, there will be no head change)
		pthread_mutex_unlock(&(list->mut));//unlock the list mutex
	}
}

void remove_value(list* list, int value) {//function remove_list, take list pointer list and integer value and remove the first instance of the value in the list
	if (!list) {//do nothing if there is no place to remove from
		return;
	}
	pthread_mutex_lock(&(list->mut));//first, we lock the list mutex, to prevent other threads from writing to the list head while we reading from it
	if (!(list->head)) {
		//do nothing cause the list is empty, just relese the list locked mutex
		pthread_mutex_unlock(&(list->mut));
	}
	else {
		node* head = list->head;//getting the head of the list
		pthread_mutex_unlock(&(list->mut));//ulocking the list mutex
		pthread_mutex_lock(&(head->mut));//lock the list head mutex
		while ((head->next) && (head->next->value < value)) {//while we have where to continue, and the next element is smaller than our value, run on the list with hand over hand locking
			pthread_mutex_lock(&(head->next->mut));//lock the next element on the list
			head = head->next;//assign head->next to be the new head
			pthread_mutex_unlock(&(head->prev->mut));//unlock the mutex of the head from the beginning of the loop
		}
		if ((!head->prev) && (head->value == value)) {//if we need to remove the first value in the list
			node* next = head->next;//get the second node from the list
			pthread_mutex_unlock(&(head->mut));//unlock the last mutex that wasn't unlocked yet 
			delete_node(head);//delete the list head
			head = next;//assign the second element on the list to be in head
			pthread_mutex_lock(&(next->mut));//lock the cuurent head mutex
			head->prev = NULL;//complitly remove any sign to the existence of the removed element 
			pthread_mutex_unlock(&(next->mut));//unlock the new head mutex
			pthread_mutex_lock(&(list->mut));//lock the list mutex to prevent any other thread from reading from/writing into the list head 
			list->head = head;//updating the list head to be the new head
			pthread_mutex_unlock(&(list->mut));//unlock the list mutex
		}
		else if (!head->next || (head->next->value > value)) {
			//do nothing, value isn't in the list, just relese locks
			pthread_mutex_unlock(&(head->mut));
		}
		else {//the node we need to remove is somewhere in the middle of the list(head->next)
			node* toRemove = head->next;//toRemove is the head->next
			if (toRemove->next) {//if toRemove isn't the last element in the list
				pthread_mutex_lock(&(toRemove->next->mut));//lock its next mutex
				toRemove->next->prev = toRemove->prev;//the prev of toRemove->next is the toRemove->prev
			}
			toRemove->prev->next = toRemove->next;//the next of toRemove->prev is toRemove->next
			pthread_mutex_unlock(&(toRemove->prev->mut));//unlock the mutex of head
			if (toRemove->next) {//if toRemove isn't the last element in the list
				pthread_mutex_unlock(&(toRemove->next->mut));//unlock its next mutex
			}
			pthread_mutex_unlock(&(toRemove->mut));//unlock toRemove mutex
			delete_node(toRemove);//delete toRemove
		}
	}
	
}

void print_list(list* list) {//function print_list, take a list* as an argument and print its elements
	if (!list) {//if there is no list, I don't see any reason to print something other than "\n"
		printf("\n");
		return;
	}
	pthread_mutex_lock(&(list->mut));//lock the list mutex to prevent other threads from write into/read from the list head while we reading it
	node* head = list->head;//getting the list head
	if (!head) {//if there was no head, I see no reason to print anything other than "\n"
		printf("\n");
		return;
	}
	pthread_mutex_unlock(&(list->mut));//unlock the list mutex
	pthread_mutex_lock(&(head->mut));//lock the list head mutex
	while (head->next) {//while we have where to continue, run on the list with hand over hand locking
		pthread_mutex_lock(&(head->next->mut));//lock the next element on the list
		print_node(head);//print head value
		head = head->next;//assign head->next to be the new head
		pthread_mutex_unlock(&(head->prev->mut));//unlock the mutex of the head from the beginning of the loop
	}
	print_node(head);//print last unprinted node
	pthread_mutex_unlock(&(head->mut));//unlock the last locked mutex
	printf("\n");
}

void count_list(list* list, int(*predicate)(int)) {
	int count = 0;
	if (!list) {//if no list, 0 items were counted for sure! 
		printf("%d items were counted\n", count);
		return;
	}
	pthread_mutex_lock(&(list->mut));//lock list mutex to prevent any other thread from reading from/writing into the list head while we reading from there
	node* head = list->head;//getting list head
	if (!head) {//if empty list, 0 items were counted for sure!
		printf("%d items were counted\n", count);
		pthread_mutex_unlock(&(list->mut));//unlock the list mutex.
		return;
	}
	pthread_mutex_unlock(&(list->mut));
	pthread_mutex_lock(&(head->mut));//lock the list head mutex
	while (head->next) {//while we have where to continue, run on the list with hand over hand locking
		pthread_mutex_lock(&(head->next->mut));//lock the next element on the list
		count += predicate(head->value);//in C there are no booleans, so if the predict will return true it will return 1 and will be counted, and if the predict return false, it will return 0 and will not be counted
		head = head->next;//assign head->next to be the new head
		pthread_mutex_unlock(&(head->prev->mut));//unlock the mutex of the head from the beginning of the loop
	}
	count += predicate(head->value);//counting/not counting last element
	pthread_mutex_unlock(&(head->mut));//unlocking last locked mutex

	printf("%d items were counted\n", count);//print number of counted items.
}
