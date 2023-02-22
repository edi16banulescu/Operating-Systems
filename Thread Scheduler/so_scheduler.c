#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"

typedef struct scheduler {
	unsigned int io;
	unsigned int quantum;
	sem_t s_sem;
	queue_t active_threads;
	prio_queue_t *prio_queue;
} scheduler_t;

scheduler_t *scheduler;

/* Search for a specific thread by tid*/
thread_t *find_thread(queue_t *q, tid_t tid);

/* Schedule a task and reset quantum if necessary */
void check_scheduler(thread_t *calling_thread);

/* Start a thread and calls the scheduler */
void *thread_routine(void *args);


DECL_PREFIX int so_init(unsigned int time_quantum, unsigned int io)
{
	/* Check params */
	if (time_quantum < 1 || scheduler != NULL)
		return -1;

	if ((io < 0) || (io > SO_MAX_NUM_EVENTS))
		return -1;

	/* Initialize my scheduler */
	scheduler = (scheduler_t *)malloc(sizeof(scheduler_t));
	scheduler->prio_queue = priq_new();
	scheduler->quantum = time_quantum;
	scheduler->io = io;
	init_list(&scheduler->active_threads);
	sem_init(&scheduler->s_sem, 0, 0);


	/* Add initial thread to thread structure, with lowest priority */
	thread_t *new = (thread_t *)malloc(sizeof(thread_t));

	new->tid = pthread_self();
	new->priority = 0;
	new->handler = NULL;
	new->thread_quantum = scheduler->quantum;
	sem_init(&new->t_sem, 0, 0);
	add_elem(&scheduler->active_threads, new);

	return 0;
}


DECL_PREFIX tid_t so_fork(so_handler *func, unsigned int priority)
{
	/* Check params */
	if ((priority > SO_MAX_PRIO) || (!func))
		return INVALID_TID;

	int rc;
	thread_t *new = (thread_t *)malloc(sizeof(thread_t));

	if (!new)
		return INVALID_TID;

	/* Create new thread structure element */
	new->priority = priority;
	new->handler = func;
	new->thread_quantum = scheduler->quantum;
	sem_init(&new->t_sem, 0, 0);

	rc = pthread_create(&new->tid, NULL, thread_routine, new);

	if (rc != 0) {
		sem_destroy(&new->t_sem);
		free(new);
		return INVALID_TID;
	}

	/* Do thread routine */
	sem_post(&scheduler->s_sem);
	sem_wait(&new->t_sem);
	(new->handler)(new->priority);

	/* Consume time on CPU */
	thread_t *this = find_thread(&scheduler->active_threads, pthread_self());

	this->thread_quantum--;
	sem_wait(&scheduler->s_sem);

	return pthread_self();
}


DECL_PREFIX int so_wait(unsigned int io)
{
	return 0;
}


DECL_PREFIX int so_signal(unsigned int io)
{
	return 0;
}


DECL_PREFIX void so_exec(void) {}


DECL_PREFIX void so_end(void)
{
	if (scheduler == NULL)
		return;

	/* Destroy first thread */
	thread_t *this = find_thread(&scheduler->active_threads, pthread_self());

	sem_destroy(&this->t_sem);

	/* Free memory */
	remove_elem(&scheduler->active_threads, this);
	sem_destroy(&scheduler->s_sem);
	priq_free(scheduler->prio_queue);
	free_list(&scheduler->active_threads);
	free(scheduler);
	scheduler = NULL;
}


thread_t *find_thread(queue_t *q, tid_t tid)
{
	node_t *p = q->first;

	if ((p->data)->tid == tid)
		return p->data;

	while (p != NULL) {
		if ((p->data)->tid == tid)
			return p->data;
		p = p->next;
	}
	return NULL;
}


void check_scheduler(thread_t *my_thread)
{
	thread_t *thread = priq_pop(scheduler->prio_queue);

	if (thread == NULL)
		return;

	if (my_thread == NULL) {
		sem_post(&thread->t_sem);
		return;
	}

	if (thread != my_thread)
		my_thread->thread_quantum = scheduler->quantum;

	if (my_thread->thread_quantum == scheduler->quantum)
		my_thread->thread_quantum = scheduler->quantum;

	sem_post(&thread->t_sem);
}


void *thread_routine(void *args)
{
	thread_t *new_thread = (thread_t *)args;

	priq_push(scheduler->prio_queue, (void *)new_thread, new_thread->priority);

	remove_elem(&scheduler->active_threads, new_thread);
	check_scheduler(NULL);

	return NULL;
}
