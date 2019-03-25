#ifndef BARRIER_H
#define BARRIER_H


#include <pthread.h>

/* Material de concorrência */
typedef struct barrier_t {
	pthread_mutex_t cond_mutex;
	pthread_cond_t wait_for_iteration;
	size_t size, count;
} barrier_t;

int barrier_init(barrier_t*, size_t);
int barrier_deinit(barrier_t*);
void barrier_lock(barrier_t*);
void barrier_unlock(barrier_t*);
int barrier_wait(barrier_t*);

#endif // BARRIER_H
