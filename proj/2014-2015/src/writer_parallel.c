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
#include <sys/time.h>
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h> /* needs -D_POSIX_C_SOURCE if using -ansi */
#include <string.h>

#include "writer_parallel.h"
#include "shared_stuff.h"
#include "writer.h"

#define BUFFER_CHAR_COUNT 32

static int use_locks = TRUE;
static int enable_writing_errors = FALSE;

/** TRUE if we should quit the program */
static int stop_writing = FALSE;

/** Handles the SIGUSR1 signal, which inverts Lock usage
 * @param sig_num the number of the calling signal
 */
void sigusr1_handler(int sig_num) {
	(void) sig_num; /* suppress unused variable warning */
	use_locks = !use_locks;
	
	if (use_locks) { 
		DBG_PRINT("Using locks.\n");
	}
	else {
		DBG_PRINT("Not using locks.\n");
	}
}

/** Handles the SIGUSR2, which inverts error writing 
 * @param sig_num the number of the calling signal
 */
void sigusr2_handler(int sig_num) {
	(void) sig_num; /* suppress unused variable warning */
	enable_writing_errors = !enable_writing_errors;
	
	if (enable_writing_errors) {
		DBG_PRINT("Writing with errors.\n");
	}
	else {
		DBG_PRINT("Writing without errors.\n");
	}
}

/** Handles the SIGTSTP signal, which terminates the program
 * @param sig_num the number of the calling signal
 */
void sigtstp_handler(int sig_num) {
	(void) sig_num; /* suppress unused variable warning */
	stop_writing = TRUE;
	
	DBG_PRINT("STOP WRITING!\n");
}

void *writer_thread(void *p) {
	(void) p; /* suppress unused variable warning */
	int file_num;
	char *rand_str;
	
	DBG_PRINT("writer_thread running.\n");
	while (!stop_writing) {
		
		file_num = RANDOM_RANGE(0, 4);
		rand_str = get_writer_string( RANDOM_RANGE(0, WRITER_STRING_COUNT - 1) );
		
		writer(file_num, rand_str, WRITER_STRING_LEN, use_locks, enable_writing_errors);

	}
	
	return NULL;
}

int run_and_wait_for_threads(int thread_count) {
	int error;
    int i;
	pthread_t *threads;
    
	threads = (pthread_t*) malloc( sizeof(pthread_t) * thread_count );
	if (threads == NULL) {
		printf("Could not allocate memory for 'threads'\n");
		return -1;
	}
	
	/* start threads */
	for (i = 0; i < thread_count; i++) {
		error = pthread_create( threads+i, NULL, writer_thread, NULL );
		if (error != 0) {
			printf("Error %d: Could not create thread %d\n", error, i);
			free(threads);
			return -1;
		}
	}
	
	printf("Writer running with %d threads.\n", thread_count);
	
	/* wait for threads */
	for (i = 0; i < thread_count; i++) {
		DBG_PRINTF("joining thread %d\n",i);
		error = pthread_join(threads[i], NULL);
		if (error != 0) {
			printf("Error %d: Thread %d could not be suspended\n", error, i);
			free(threads);
			return -1;
		}
	}
	free(threads);
	return 0;
}


int main() {
	int ret;
	struct timeval tvstart;     /* data de inicio */
	struct timeval tvend;       /* data de fim */
	struct timeval tvduration;  /* diferenca entre as duas datas */
	unsigned int duration_us;   /* diferenca entre as datas em microssegundos */
	
	time_t curtime;             /* tempo em formato time_t para conversao de formatos */
	char buffer[BUFFER_CHAR_COUNT]; /* para escrever a data em formato legivel */
	
	/* signals struct declarations */
	struct sigaction new_action; 
	
	DBG_PRINT("main()\n");	
	/* --- register SIGNAL handlers --- */
	
	/* register SIGUSR1 handler */
	new_action.sa_handler = sigusr1_handler;
	sigemptyset (&new_action.sa_mask);
	sigaddset(&new_action.sa_mask, SIGUSR1);
	new_action.sa_flags = 0;
	sigaction( SIGUSR1, &new_action, NULL);
	
	/* register SIGUSR2 handler */
	new_action.sa_handler = sigusr2_handler; /* */
	sigemptyset(&new_action.sa_mask); /* clear the set of blocked signals */
	sigaddset(&new_action.sa_mask, SIGUSR2); /* add SIGUSR2 to blocked sigs */
	new_action.sa_flags = 0; /* don't modify signals behaviour */
	sigaction(SIGUSR2, &new_action, NULL); /* bind the action to SIGUSR2 */
	
	/* register SIGTSTP handler */
	new_action.sa_handler = sigtstp_handler;
	sigemptyset (&new_action.sa_mask);
	sigaddset(&new_action.sa_mask, SIGTSTP);
	new_action.sa_flags = 0;
	sigaction( SIGTSTP, &new_action, NULL);
	
	/* --- END registering SIGNAL handlers --- */
	
	
	/* print current time */
	
	gettimeofday(&tvstart, NULL);   /* ler a data actual */
	/* converter e imprimir a data */
	curtime = tvstart.tv_sec;
	strftime(buffer, BUFFER_CHAR_COUNT, "%m-%d-%Y  %T.", localtime(&curtime));
	printf("Writer: started on %s%ld\n", buffer, (long int)tvstart.tv_usec);
	
	
	ret = run_and_wait_for_threads(THREAD_COUNT);
	if (ret < 0) /* error */
		return ret;
	
	
	gettimeofday(&tvend, NULL); /* ler a data actual */
	
	/* converter e imprimir a data */
	curtime = tvend.tv_sec;
	strftime(buffer, BUFFER_CHAR_COUNT, "%m-%d-%Y  %T.", localtime(&curtime));
	printf("Writer: ended on %s%ld\n", buffer, (long int)tvend.tv_usec);
	
	/* calcular e imprimir a diferenca de datas */
	tvduration.tv_sec = tvend.tv_sec - tvstart.tv_sec;
	tvduration.tv_usec = tvend.tv_usec - tvstart.tv_usec;
	duration_us = tvduration.tv_sec * 1000000 + tvduration.tv_usec;
	printf("Writer: rand for %.4f segundos\n", ((float)duration_us)/1000000.0f);
	
	
	return ret;
}
