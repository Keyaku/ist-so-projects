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

#ifndef __WRITER_PARALLEL_H__
	#define __WRITER_PARALLEL_H__
	
    #define THREAD_COUNT 10
    
	/**
	 * @brief Runs writer threads and waits for them to finish.
	 * 
	 * @param thread_count number of child processes
	 * 
	 * @retval -1 on failure, 0 on success
	 */
	int run_and_wait_for_threads( int thread_count );

	void *writer_thread(void *p);
	
	void sigusr1_handler(int sig_num);
	void sigusr2_handler(int sig_num);
	void sigtstp_handler(int sig_num);
	
#endif /* __WRITER_PARALLEL_H__ */
