#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>

#ifdef _WIN32
	#include <windows.h>
	#define sleep_ms(s) Sleep(s)
#else
	#include <unistd.h>
	#define sleep_ms(s) usleep(1000*(s))
#endif


#include "queue.h"


struct node {
	void        *data;
	struct node *next;
};


struct queue {
	pthread_cond_t   cond;
	pthread_mutex_t  lock;
	struct node     *back;
	struct node     *front;
	size_t           elemsize;
	size_t           elems;
	int              finish;
};


queue* queue_create(size_t elemsize)
{
	queue *q;

	q = calloc(1, sizeof(*q));
	if (NULL == q) {
		perror("queue");
		return NULL;
	}

	pthread_cond_init(&q->cond, NULL);
	pthread_mutex_init(&q->lock, NULL);
	q->elemsize = elemsize;

	return q;
}


int queue_free(queue *q)
{
	if (NULL == q)
		return 0;
	pthread_mutex_lock(&q->lock);
	q->finish = 0;
	pthread_cond_signal(&q->cond);
	pthread_mutex_unlock(&q->lock);
	sleep_ms(200);
	free(q);
	return 0;
}


size_t queue_size(queue *q)
{
	size_t l;
	pthread_mutex_lock(&q->lock);
	l = q->elems;
	pthread_mutex_unlock(&q->lock);
	return l;
}


int queue_enqueue(queue *q, const void *elem)
{
	struct node *n;

	assert(NULL != q && "q cannot be NULL");

	n = calloc(1, sizeof(*n));
	if (NULL == n) {
		perror("calloc");
		return -1;
	}

	n->data = malloc(q->elemsize);
	if (NULL == n->data) {
		perror("malloc");
		free(n);
		return -1;
	}
	memcpy(n->data, elem, q->elemsize);

	pthread_mutex_lock(&q->lock);
		if (NULL == q->front) {
			q->front = n;
			q->back  = n;
		} else {
			q->back->next = n;
			q->back = n;
		}
		q->elems++;
	pthread_cond_signal(&q->cond);
	pthread_mutex_unlock(&q->lock);

	return 0;
}


int queue_dequeue(queue *q, void *elem)
{
	struct node *n = NULL;

	pthread_mutex_lock(&q->lock);
	while (!q->elems && !q->finish)
		pthread_cond_wait(&q->cond, &q->lock);


	if (q->finish) {
		pthread_mutex_unlock(&q->lock);
		return -1;
	}
	n = q->front;
	q->front = q->front->next;
	q->elems--;


	pthread_mutex_unlock(&q->lock);

	memcpy(elem, n->data, q->elemsize);
	free(n->data);
	free(n);
	return 0;
}
