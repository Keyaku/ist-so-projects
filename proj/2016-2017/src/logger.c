/*
** Logger
** Sistemas Operativos, Antonio Sarmento, 77906, 2016-2017
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "command.h"

/* General material */
#define BUFF_LOG_SIZE 256
char buffer_log[BUFF_LOG_SIZE];

/* File data */
#define LOG_FILENAME "log.txt"

int fd_logger;

/******************************** Private ***********************************/

/******************************** Public ************************************/
/* Constructor & destructor for logger */
void init_logger(void)
{
	int flags = O_RDWR | O_CREAT | O_TRUNC;
	int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	fd_logger = open(LOG_FILENAME, flags, perms);
	if (fd_logger == -1) {
		perror("Couldn't open file for logger");
		exit(EXIT_FAILURE);
	}
}

void destroy_logger(void)
{
	close(fd_logger);
}

void write_to_log(unsigned long thread, const char *str)
{
	/* Formatting information */
	snprintf(buffer_log, sizeof(buffer_log), "%lu", thread);
	snprintf(buffer_log, sizeof(buffer_log), "%s\n", str);

	/* Writing to file */
	write(fd_logger, buffer_log, sizeof(buffer_log));
}
