#include <stdio.h>
#include <stdlib.h>

#include "barrier.h"


/* Helpers for (un)locking mutexes */
void lock_or_exit(pthread_mutex_t *mutex) {
	if (pthread_mutex_lock(mutex)) {
		fprintf(stderr, "\nErro ao bloquear mutex\n");
		exit(EXIT_FAILURE);
	}
}

void unlock_or_exit(pthread_mutex_t *mutex) {
	if (pthread_mutex_unlock(mutex)) {
		fprintf(stderr, "\nErro ao desbloquear mutex\n");
		exit(EXIT_FAILURE);
	}
}

/*--------------------------------------------------------------------
| Functions: Initializing and cleaning multithreading material
| - barrier_init(barrier, size)
|   | Initializes given barrier with given size
|
| - barrier_deinit(barrier)
|   | Destroys given barrier
|
| - barrier_wait(barrier, tid)
|   | Waits with given barrier.
|   | returns true (1) if all threads were unlocked, false (0) otherwise
---------------------------------------------------------------------*/
int barrier_init(barrier_t *barrier, size_t size) {
	if (pthread_mutex_init(&barrier->cond_mutex, NULL)) {
		fprintf(stderr, "\nErro ao inicializar wait_mutex\n");
		return 1;
	}

	if (pthread_cond_init(&barrier->wait_for_iteration, NULL)) {
		fprintf(stderr, "\nErro ao inicializar variável de condição\n");
		return 2;
	}

	barrier->size = size;
	barrier->count = 0;
	return 0;
}

int barrier_deinit(barrier_t *barrier) {
	if (pthread_mutex_destroy(&barrier->cond_mutex)) {
		fprintf(stderr, "\nErro ao destruir wait_mutex\n");
		return 1;
	}

	if (pthread_cond_destroy(&barrier->wait_for_iteration)) {
		fprintf(stderr, "\nErro ao destruir variável de condição\n");
		return 2;
	}

	return 0;
}

void barrier_lock(barrier_t *barrier) {
	lock_or_exit(&barrier->cond_mutex);
}

void barrier_unlock(barrier_t *barrier) {
	unlock_or_exit(&barrier->cond_mutex);
}

int barrier_wait(barrier_t *barrier) {
	int retval = 0;

	barrier->count++;

	/* Se o contador for inferior ao tamanho, bloquear thread */
	if (barrier->count < barrier->size) {
		if (pthread_cond_wait(&barrier->wait_for_iteration, &barrier->cond_mutex)) {
			fprintf(stderr, "\nErro ao esperar pela variável de condição\n");
			exit(EXIT_FAILURE);
		}
	}
	/* Caso contrário, desbloqueia todas as threads */
	else {
		barrier->count = 0;
		if (pthread_cond_broadcast(&barrier->wait_for_iteration)) {
			fprintf(stderr, "\nErro ao desbloquear variável de condição\n");
			exit(EXIT_FAILURE);
		}

		retval = 1;
	}

	return retval;
}
