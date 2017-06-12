/*
 * thread-safe queue
 */
#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>


typedef struct queue queue;

queue* queue_create(size_t elemsize);
int    queue_free(queue *q);
size_t queue_size(queue *q);
int    queue_enqueue(queue *q, const void *elem);
int    queue_dequeue(queue *q,       void *elem);


#endif /* !QUEUE_H */
