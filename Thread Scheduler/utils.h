#include "so_scheduler.h"
#include <semaphore.h>

typedef struct thread {
	tid_t tid;
	unsigned int priority;
	so_handler *handler;
	int status;
	unsigned int thread_quantum;
	sem_t t_sem;
} thread_t;

typedef struct q_elem {
	struct q_elem *next;
	void *data;
	int pri;
} q_elem_t;

typedef struct prip_queue {
	q_elem_t *first;
	int size;
} prio_queue_t;

typedef struct node {
	struct node *next;
	thread_t *data;
	//int prio;
} node_t;

/* List structure */
typedef struct queue {
	node_t *first;
	node_t *last;
} queue_t;


/* Initialize and alloc queue */
prio_queue_t *priq_new(void);

/* Push element into the queue on the corresponding position */
void priq_push(prio_queue_t *q, void *data, int pri);

/* Remove first item. returns NULL if empty */
void *priq_pop(prio_queue_t *q);

/* Get the first element without removing it from queue */
void *priq_top(prio_queue_t *q);

/* Free queue memory */
void priq_free(prio_queue_t *q);

/* Initialize the list. Must be called before any other operation */
void init_list(queue_t *q);

thread_t *pop(queue_t *q);

/* Empty the list and free memory */
void free_list(queue_t *q);

/* Return 1 if list is empty, 0 otherwise */
int is_empty(queue_t *q);

/* Add an element at the end of the list */
void add_elem(queue_t *q, thread_t *data);

/* Remove the given element from the list */
void remove_elem(queue_t *q, thread_t *data);
