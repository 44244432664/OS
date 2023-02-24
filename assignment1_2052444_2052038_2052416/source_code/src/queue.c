#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */
	if(q->size < MAX_QUEUE_SIZE){
		q->proc[q->size] = proc;
		q->size++;
	}
	return NULL;
}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose priority is the highest
	 * in the queue [q] and remember to remove it from q
	 * */

	int pos = 0;
	struct pcb_t* dump = NULL;
	if(q->size > 0){
		int max = q->proc[0]->priority;
	for(int i = 1; i < q->size; i++){
		if(q->proc[i]->priority > max){
			max = q->proc[i]->priority;
			pos = i;
		}
	}
	dump = q->proc[pos];
	//delete(q->proc[pos]);
	q->proc[pos] = q->proc[q->size-1];
	/*
	for(int i = pos; i < q->size - 1; i++){
		q->proc[i] = q->proc[i + 1];
	}
	*/
	q->size--;
	}
	return dump;
}

