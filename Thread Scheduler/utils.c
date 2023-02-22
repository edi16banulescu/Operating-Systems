#include <stdlib.h>
#include "utils.h"
#include "so_scheduler.h"

#define NEW 1
#define READY 2
#define RUNNING 3
#define WAITING 4
#define TERMINATED 5


prio_queue_t *priq_new()
{
	prio_queue_t *pq = (prio_queue_t *)malloc(sizeof(prio_queue_t));

	pq->size = 0;
	pq->first = NULL;
	return pq;
}


void priq_push(prio_queue_t *q, void *data, int pri)
{

	if (q->size == 0) {
		q->size++;
		q->first = (q_elem_t *)malloc(sizeof(q_elem_t));
		q->first->data = data;
		q->first->pri = pri;
		q->first->next = NULL;
		return;
	}
	q->size++;

	q_elem_t *new_elem = (q_elem_t *)
								malloc(sizeof(q_elem_t));	
	new_elem->data = data;
	new_elem->pri = pri;

	q_elem_t *aux = q->first;

	if (aux->pri < pri) {
		new_elem->next = aux;
		q->first = new_elem;
		return;
	}

	q_elem_t *prev = aux;

	aux = aux->next;

	while ((aux != NULL) && (aux->pri >= pri)) {
		prev = aux;
		aux = aux->next;
	}

	new_elem->next = aux;
	prev->next = new_elem;

}


void *priq_pop(prio_queue_t *q)
{
	if (q->size == 0)
		return NULL;
	q->size--;
	q_elem_t *aux = q->first;
	void *data = aux->data;

	q->first = aux->next;
	free(aux);
	return data;
}


void *priq_top(prio_queue_t *q)
{
	if (q->size == 0)
		return NULL;
	return q->first->data;
}


void priq_free(prio_queue_t *q)
{
	while (q->size > 0)
		priq_pop(q);
	free(q);
}

void init_list(queue_t *q)
{
	q->first = NULL;
	q->last = NULL;
}

thread_t *pop(queue_t *q)
{
	node_t *p;
	thread_t *aux;

	if (q->first == NULL)
		return NULL;
	p = q->first;
	q->first = p->next;

	aux = p->data;
	return aux;
}

void free_list(queue_t *q)
{
	while (q->first != NULL) {
		node_t *p;

	if (q->first == NULL)
		return;
	p = q->first;
	q->first = (q->first)->next;

	free(p->data);
	free(p);
	}

	q->first = NULL;
	q->last = NULL;
}

int is_empty(queue_t *q)
{
	if (q->first == NULL)
		return 1;
	return 0;
}

void add_elem(queue_t *q, thread_t *data)
{
	node_t *p;

	p = (node_t *)malloc(sizeof(node_t));
	p->data = data;
	p->next = NULL;
	if (q->last != NULL)
		q->last->next = p;
	q->last = p;
	if (q->first == NULL)
		q->first = p;
}

void remove_elem(queue_t *q, thread_t *data)
{
	node_t *p = q->first;

	if (p == NULL)
		return;

	if ((p->next == NULL) && (p->data == data)) {
		free(p->data);
		free(p);
		q->first = NULL;
		q->last = NULL;
	} else {
		node_t *prev = p;

		do {
			if (p->data == data) {
				prev->next = p->next;
				if (q->first == p)
					q->first = p->next;
				if (q->last == p)
					q->last = prev;
				free(p->data);
				free(p);
				break;
			}
			prev = p;
			p = p->next;
		} while (p != NULL);
	}

}
