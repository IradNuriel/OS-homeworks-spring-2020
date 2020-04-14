#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"


node* create_node(int value);
void delete_node(node* node);


struct node {
	int value;
	node* next;
	node* prev;
	pthread_mutex_t mut;
	// add more fields
};



struct list {
	node* head;
	pthread_mutex_t mut;

	// add fields

};

void delete_node(node* node) {
	pthread_mutex_destroy(&(node->mut));
	free(node);
}

node* create_node(int value) {
	node* nod = (node*)malloc(sizeof(node));
	if (!nod) {
		printf("Error! not enough memory");
		exit(1);
	}
	nod->value = value;
	nod->next = NULL;
	nod->prev = NULL;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
	pthread_mutex_init(&(nod->mut), &attr);
	pthread_mutexattr_destroy(&attr);
	return nod;
}

void print_node(node* node) {
	// DO NOT DELETE
	if (node) {
		pthread_mutex_lock(&(node->mut));
		printf("%d ", node->value);
		pthread_mutex_unlock(&(node->mut));
	}
}

list* create_list() {
	list* lis = (list*)malloc(sizeof(list));
	if (!lis) {
		printf("ERROR! not enough memory");
		exit(1);
	}
	lis->head = NULL;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
	pthread_mutex_init(&(lis->mut), &attr);
	pthread_mutexattr_destroy(&attr);
	return lis;
}

void delete_list(list* list) {
	pthread_mutex_lock(&(list->mut));
	node* head = list->head;
	pthread_mutex_unlock(&(list->mut));
	if (!head) {
		pthread_mutex_destroy(&(list->mut));
		free(list);
		return;
	}
	pthread_mutex_lock(&(head->mut));
	node* next = head->next;
	pthread_mutex_unlock(&(head->mut));
	while (head->next) {
		pthread_mutex_lock(&(head->mut));
		pthread_mutex_lock(&(head->next->mut));
		next = head->next;
		pthread_mutex_unlock(&(next->mut));
		pthread_mutex_unlock(&(head->mut));
		delete_node(head);
		head = next;
	}
	delete_node(head);
	pthread_mutex_destroy(&(list->mut));
	free(list);
}

void insert_value(list* list, int value) {
	if (!list) {
		printf("you need to first initialize a list with create_list\n");
		return;
	}
	if (!(list->head)) {
		pthread_mutex_lock(&(list->mut));
		list->head = create_node(value);
		pthread_mutex_unlock(&(list->mut));

	}
	else {

		pthread_mutex_lock(&(list->mut));
		node* head = list->head;
		pthread_mutex_unlock(&(list->mut));
		while ((head->next) && (head->next->value <= value)) {
			pthread_mutex_lock(&(head->next->mut));
			head = head->next;
			pthread_mutex_unlock(&(head->mut));
		}
		node* n = create_node(value);
		if (!(head->next) && (n->value >= head->value)) {
			pthread_mutex_lock(&(head->mut));
			head->next = n;
			n->prev = head;
			pthread_mutex_unlock(&(head->mut));
		}
		else if (!(head->prev) && (n->value < head->value)) {
			pthread_mutex_lock(&(head->mut));
			head->prev = n;
			n->next = head;
			pthread_mutex_unlock(&(head->mut));
		}
		else {
			pthread_mutex_lock(&(head->mut));
			pthread_mutex_lock(&(head->next->mut));
			head->next->prev = n;
			n->next = head->next;
			n->prev = head;
			head->next = n;
			pthread_mutex_unlock(&(n->next->mut));
			pthread_mutex_unlock(&(head->mut));
		}
		while (head->prev) {
			head = head->prev;
		}
		pthread_mutex_lock(&(list->mut));
		list->head = head;
		pthread_mutex_unlock(&(list->mut));
	}
}

void remove_value(list* list, int value) {
	if (!list) {
		printf("you need to first initialize a list with create_list\n");
		return;
	}
	if (!(list->head)) {
		//do nothing cause the list is empty
	}
	else {

		pthread_mutex_lock(&(list->mut));
		node* head = list->head;
		pthread_mutex_unlock(&(list->mut));
		while ((head->next) && (head->next->value < value)) {
			pthread_mutex_lock(&(head->next->mut));
			head = head->next;
			pthread_mutex_unlock(&(head->mut));
		}
		if ((!head->prev) && (head->value == value)) {
			pthread_mutex_lock(&(head->next->mut));
			node* next = head->next;
			pthread_mutex_unlock(&(head->next->mut));
			delete_node(head);
			head = next;
			pthread_mutex_lock(&(list->mut));
			list->head = head;
			pthread_mutex_unlock(&(list->mut));
		}
		else if (!head->next || (head->next->value > value)) {
			//do nothing, value isn't in the list
		}
		else {
			pthread_mutex_lock(&(head->next->mut));
			node* toRemove = head->next;
			pthread_mutex_lock(&(toRemove->next->mut));
			pthread_mutex_lock(&(toRemove->prev->mut));
			toRemove->next->prev = toRemove->prev;
			toRemove->prev->next = toRemove->next;
			pthread_mutex_unlock(&(toRemove->prev->mut));
			pthread_mutex_unlock(&(toRemove->next->mut));
			pthread_mutex_unlock(&(toRemove->mut));
			delete_node(toRemove);
		}
	}
	// add code here
}

void print_list(list* list) {
	// add code here
	if (!list) {
		return;
	}
	pthread_mutex_lock(&(list->mut));
	node* head = list->head;
	if (!head) {
		return;
	}
	pthread_mutex_unlock(&(list->mut));
	while (head->next) {
		print_node(head);
		pthread_mutex_lock(&(head->next->mut));
		head = head->next;
		pthread_mutex_unlock(&(head->mut));
	}
	print_node(head);
	printf("\n"); // DO NOT DELETE
}

void count_list(list* list, int(*predicate)(int)) {
	int count = 0; // DO NOT DELETE
	if (!list) {
		return;
	}
	pthread_mutex_lock(&(list->mut));
	node* head = list->head;
	if (!head) {
		return;
	}
	pthread_mutex_unlock(&(list->mut));
	while (head->next) {
		pthread_mutex_lock(&(head->mut));
		count += predicate(head->value);
		pthread_mutex_unlock(&(head->mut));
		pthread_mutex_lock(&(head->next->mut));
		head = head->next;
		pthread_mutex_unlock(&(head->mut));
	}
	pthread_mutex_lock(&(head->mut));
	count += predicate(head->value);
	pthread_mutex_unlock(&(head->mut));

	printf("%d items were counted\n", count); // DO NOT DELETE
}
