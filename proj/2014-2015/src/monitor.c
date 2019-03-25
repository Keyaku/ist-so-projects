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

#include "monitor.h"
#include "shared_stuff.h"

#include <unistd.h>
#include <signal.h> /* needs -D_POSIX_C_SOURCE if using -ansi */
#include <sys/wait.h>
#include <sys/errno.h>
#include <string.h>
#include <stdlib.h>

static int writer_pid = 0;
static int reader_pid = 0;
static int reader_pipe_fd[2];

int run_process(char* prog_path, int* stdin_pipe_fd) {
	int res;
	res = fork();

	if (res == 0) {
		/* child */
		if (stdin_pipe_fd != NULL) {
			close(STDIN_FILENO); /* close the normal stdin */
			dup(stdin_pipe_fd[0]); /* duplicate the reading end of the pipe */
			close(stdin_pipe_fd[0]); /* not needed anymore -> it's stdin now */
			close(stdin_pipe_fd[1]); /* not needed anymore -> parent writing end */
		}
		res = execl(prog_path, prog_path, (const char*)NULL);
		if( res == -1 ){
			printf("execl failed. errno=%d\n", errno);
		}
	}
	else if (res == -1) {
		/* error */
		printf("fork failed. errno=%d\n", errno);
	}
	else {
		/* parent */
		return res;
	}
	return -1;
}

void exit_monitor() {
	if ( !writer_pid || !reader_pid ) {
		/* something went wrong here... */
		DBG_PRINT("invalid children pid's\n");
		exit(-1);
	}

	/* EOF in reader */
	close(reader_pipe_fd[0]);
	close(reader_pipe_fd[1]);

	/* send SIGTSTP to writer */
	kill(writer_pid, SIGTSTP);
	
	DBG_PRINT("waiting for writer\n");
	if (waitpid(writer_pid, NULL, 0) == -1) {
		DBG_PRINT("failed waiting on writer_pid\n");
		exit(-1);
	}

	DBG_PRINT("waiting for reader\n");
	if (waitpid(reader_pid, NULL, 0) == -1){
		 DBG_PRINT("failed waiting on reader_pid\n");
		 exit(-1);
	}
	DBG_PRINT("exit_monitor() return\n");
}

int main(void) {
	int ret;
	int quit = FALSE;
	char input_buffer[INPUT_BUFFER_SIZE];
	/* pipe for reder's stdin */
	if (pipe(reader_pipe_fd) == -1) {
		DBG_PRINT("Error while creating the pipe\n");
	}

	writer_pid = run_process((char*)WRITER_PATH, NULL );
	if(writer_pid == -1) {
		printf("Could not start writer.\n");
		exit(-1);
	}

	/* workaround for #19; see #20 too */
	sleep(1);

	reader_pid = run_process((char*)READER_PATH, reader_pipe_fd);
	if(reader_pid == -1) {
		printf("Could not start reader.\n");
		exit(-1);
	}

	DBG_PRINTF("writer_pid=%d, reader_pid=%d\n", writer_pid, reader_pid);

	while (!quit) {
		ret = read_command_from_fd(STDIN_FILENO, input_buffer, INPUT_BUFFER_SIZE);
		if( ret != 0 ) {
			printf("Monitor will quit.\n");
			break;
		}

		DBG_PRINTF("input = '%s'\n", input_buffer);

		quit = process_command(input_buffer);
	}

	exit_monitor();
	return 0;
}

int process_command(char* command) {
	DBG_PRINTF("command = %s\n", command);

	if (!strcmp(command, EXIT_COMMAND)) {
		return 1;
	}
	else if (!strcmp(command, SIGUSR1_COMMAND)) {
		kill(writer_pid, SIGUSR1);
	}
	else if (!strcmp(command, SIGUSR2_COMMAND)) {
		kill(writer_pid, SIGUSR2);
	}
	else { /* send command to writer */
		write(reader_pipe_fd[1], command, strlen(command)*sizeof(char));
		write(reader_pipe_fd[1], LINE_FEED, LINE_FEED_LEN*sizeof(char));
	}
	return 0;
}
