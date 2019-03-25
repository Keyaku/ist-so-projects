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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#include "reader.h"
#include "reader_parallel.h"
#include "shared_stuff.h"
#include "shared_buffer.h"


static shared_buffer_t Item_Buffer;

void *reader_thread(void *arg) {
	char * filename;
	int result;
	(void) arg; /* suppress unused variable warning */

	DBG_PRINT("Reader thread running\n");
	while(1) {
		filename = shared_buffer_consume(&Item_Buffer);
		if( filename == NULL) {
			DBG_PRINT("Thread exiting\n");
			break;
		}
		
		DBG_PRINTF("Thread reading %s\n", filename);
		
		/* process the file */
		result = reader(filename);
		if( result == FILE_IS_INVALID ) {
			printf("'%s' is invalid.\n", filename);
		}
		else {
			printf("'%s' is valid.\n", filename);
		}
		
		free(filename);
	}
	return NULL;
}

int run_threads(pthread_t **threads, int thread_count) {
	int i,
		error;
	if (threads == NULL) {
		return 0;
	}
	
	*threads = (pthread_t*) malloc( sizeof(pthread_t) * thread_count );
	if (*threads == NULL) {
		free(*threads);
		printf("Could not allocate memory for 'threads'\n");
		return -1;
	}
	
	/* start threads */
	for (i = 0; i < thread_count; i++) {
		DBG_PRINTF("Firing up thread %d.\n", i);
		
		error = pthread_create( (*threads)+i, NULL, reader_thread, NULL );
		if (error != 0) {
			printf("Error %d: Could not create thread %d\n", error, i);
			free(*threads);
			return -1;
		}
	}
	
	return 0;
}

int wait_for_threads(pthread_t **threads, int thread_count) {
	int i,
		error;
	
	/* wait for threads */
	for (i = 0; i < thread_count; i++) {
		error = pthread_join( (*threads)[i], NULL );
		if (error != 0) {
			printf("Error %d: Thread %d could not be suspended\n", error, i);
			free(*threads);
			return -1;
		}
	}
	
	free(*threads);
	return 0;
}

int main(void) {
	int i;
	int ret;
	char *input_buffer;
	struct timeval time_now;
	pthread_t *threads;
	
	/* use the current micro seconds as a random seed */
	gettimeofday(&time_now, NULL);
	srand(time_now.tv_usec);

	/* Initialize shared buffer */
 	if ( shared_buffer_init(&Item_Buffer, 0, ITEM_BUFFER_SIZE) != 0 ) {
		printf("Could not allocate shared buffer.\n");
		exit(-1);
	}

	/*	Launching threads BEFORE capturing any input	*/
	if( run_threads( &threads, READER_THREAD_COUNT ) != 0 ) {
		printf("Could run threads.\n");
		exit(-1);
	}

	/* allocate input_buffer */
	input_buffer = (char*) malloc( INPUT_BUFFER_SIZE );
	if( input_buffer == NULL) {
		printf("Could not allocate input buffer.\n");
		exit(-1);
	}
	
	printf("Reader running with %d threads.\n", READER_THREAD_COUNT);
	while (TRUE) {
		ret = read_command_from_fd(STDIN_FILENO, input_buffer, INPUT_BUFFER_SIZE);
		if( ret != 0 ) {
			printf("Reader quit.\n");
			break;
		}
		
		DBG_PRINTF("input = '%s'\n", input_buffer);
		
		process_file(input_buffer);
	}
	
	free(input_buffer);
	
	
	/* make threads quit (insert READER_THREAD_COUNT NULLs) */
	for ( i = 0; i < READER_THREAD_COUNT; i++ ) {
		shared_buffer_insert( &Item_Buffer, (item_t) NULL );
	}
	
	wait_for_threads( &threads, READER_THREAD_COUNT );
	
	shared_buffer_close(&Item_Buffer);
	
	return EXIT_SUCCESS;
}


void process_file( char* filename ) {
	char* item = (char*) malloc( (strlen(filename) + 1) * sizeof(char) );
	if(item == NULL) {
		printf("Could not allocate memory.\n");
		return;
	}
	strcpy(item, filename);
	DBG_PRINTF("process_file insert item %s\n", item);
	shared_buffer_insert( &Item_Buffer, (item_t) item );
}