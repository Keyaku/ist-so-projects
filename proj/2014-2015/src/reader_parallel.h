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

#ifndef __READER_PARALLEL_H__
	#define __READER_PARALLEL_H__

	#define READER_THREAD_COUNT 3
	
	#define ITEM_BUFFER_SIZE	32
	#define INPUT_BUFFER_SIZE	( sizeof(char) * 255 )
	

	void *reader_thread(void *arg);

	/**
	* @brief Runs reader threads, each one reading a different file brought to
	* it via a shared buffer.
	* 
	* @param threads Pointer to the thread array that receives the created threads.
	* @param thread_count Number of threads to run
	* 
	* @retval -1 on failure, 0 on success
	*/
	int run_threads(pthread_t **threads, int thread_count);
	
	/**
	* @brief Waits for running threads to finish.
	* 
	* @param threads Pointer to the thread array that conteins the threads to wait for
	* @param thread_count Number of threads in the array
	*
	* @retval -1 on failure, 0 on success
	*/
	int wait_for_threads(pthread_t **threads, int thread_count);
	
	
	/**
	 * Inserts the given file in the shared buffer
	 */
	void process_file( char* filename );

	typedef struct {
		int first_line, last_line;
		char *filename;
	} thread_info_t;

#endif /* __READER_PARALLEL_H__ */
