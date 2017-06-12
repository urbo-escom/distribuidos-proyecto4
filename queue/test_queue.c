#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "queue.h"

#ifdef _WIN32
	#include <windows.h>
	#define sleep_ms(s) Sleep(s)
#else
	#include <unistd.h>
	#define sleep_ms(s) usleep(1000*(s))
#endif


void* proc(void *v) {
	struct queue *q = v;
	int num;
	int s;

	while (1) {
		s = queue_dequeue(q, &num);
		if (-1 == s || -1 == num)
			return NULL;
		printf("thread %d: task(%d) start\n", pthread_self(), num);
		sleep_ms(1500);
	}

	return NULL;
}


int main()
{
	#define NUM_THREADS 5
	pthread_t thread[NUM_THREADS];
	size_t    threadlen = NUM_THREADS;
	queue *q;
	size_t i;
	int n;


	q = queue_create(sizeof(int));
	if (NULL == q) {
		perror("queue_create");
		return EXIT_FAILURE;
	}


	for (i = 0; i < threadlen; i++) {
		if (-1 == pthread_create(&thread[i], NULL, proc, q)) {
			perror("pthread_create");
			queue_free(q);
			return EXIT_FAILURE;
		}
	}


	for (i = 0; i < 4*threadlen; i++) {
		n = i;
		printf("Enqueuing %d\n", n);
		queue_enqueue(q, &n);
	}

	n = -1;
	sleep_ms(7000);
	for (i = 0; i < threadlen; i++)
		queue_enqueue(q, &n);
	sleep_ms(1000);

	for (i = 0; i < threadlen; i++)
		pthread_join(thread[1], NULL);

	queue_free(q);
	return EXIT_SUCCESS;
}
