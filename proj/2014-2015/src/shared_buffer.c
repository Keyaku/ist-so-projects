/*
 * This file is part of proj-so.
 *
 * Copyright (C) 2014 Antonio Sarmento, Illya Gerasymchuk, Nuno Silva. All
 * Rights Reserved.
 *
 * proj-so is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * proj-so is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "shared_buffer.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

void shared_buffer_insert( shared_buffer_t *s, item_t i ) {
	
	/* lock if there are no empty slots */
	if (sem_wait(&(s->empty)) != 0) {
		printf("Error waiting on semaphore. errno=%d\n", errno);
		exit(-1);
	}
	
	if (pthread_mutex_lock(&(s->mutex)) !=0 ) {
		printf("Error locking the mutex\n");
		exit(-1);
	}
	
	s->buffer[s->index++] = i; /* insert the item */
	
	if (pthread_mutex_unlock(&(s->mutex)) !=0) {
		printf("Error unlocking the mutex\n");
		exit(-1);
	}
	
	if (sem_post(&(s->occupied)) != 0) {
		printf("Error posting on semaphore.\n");
		exit(-1);
	}
	
}

item_t shared_buffer_consume( shared_buffer_t *s ) {
	
	item_t item = NULL;
	
	/* lock if there are no available items */
	if (sem_wait(&(s->occupied)) != 0) {
		printf("Error waiting on semaphore\n");
		exit(-1);
	}
	
	if (pthread_mutex_lock(&(s->mutex)) !=0 ) {
		printf("Error locking the mutex\n");
		exit(-1);
	}
	
	item = s->buffer[--(s->index)];
	
	
	if (pthread_mutex_unlock(&(s->mutex)) !=0 ) {
		printf("Error unlocking the mutex\n");
		exit(-1);
	}
	
	/* there is another free slot now */
	if (sem_post(&(s->empty)) != 0) {
		printf("Error posting on semaphore\n");
		exit(-1);
	}
	
	return item;
	
}

int shared_buffer_init( shared_buffer_t *s, int pshared_val, size_t size ) {
	
	s->buffer = (item_t*) malloc( sizeof(item_t) * size);
	if( s->buffer == NULL) {
		printf("Error allocating shared buffer\n");
		return -1;
	}
	
	/* initially all slots are empty */
	if (sem_init(&(s->empty), pshared_val, size) == -1) {
		printf("Error initliazing *empty* semaphore. errno=%d\n", errno);
		return -1;
	}
	
	/* intially there are no occupied slots */
	if (sem_init(&(s->occupied), pshared_val, 0) == -1) {
		printf("Error initliazing *occupied* semaphore. errno=%d\n", errno);
		return -1;
	}
	
	pthread_mutex_init(&(s->mutex), NULL);
	
	s->index = 0;
	
	s->size = size;

	return 0;
}

int shared_buffer_close(shared_buffer_t *s ) {
	
	if (sem_destroy(&(s->occupied)) == -1) {
		printf("Error closing *occupied* semaphore. errno=%d\n", errno);
		return -1;
	}
	
	if (sem_destroy(&(s->empty)) == -1) {
		printf("Error closing *empty* semaphore. errno=%d\n", errno);
		return -1;
	}
	
	if (pthread_mutex_destroy(&(s->mutex)) != 0) {
		printf("Error destroying *mutex*\n");
		return -1;
	} 
	
	free( s->buffer );
	return 0;
}
	

