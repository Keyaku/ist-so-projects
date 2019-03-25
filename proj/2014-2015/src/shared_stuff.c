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

#include "shared_stuff.h"
#include <unistd.h>

char writer_strings [ WRITER_STRING_COUNT ][ WRITER_STRING_LEN + 1 ] = {
	"aaaaaaaaa\n",
	"bbbbbbbbb\n",
	"ccccccccc\n",
	"ddddddddd\n",
	"eeeeeeeee\n",
	"fffffffff\n",
	"ggggggggg\n",
	"hhhhhhhhh\n",
	"iiiiiiiii\n",
	"jjjjjjjjj\n"
};


int read_command_from_fd(int fd, char * buff, size_t buff_size) {
	size_t count = 0;
	int read_res;
	while( count < buff_size ) {
		/* read byte-by-byte */
		read_res = read(fd, buff+count, 1);

		if (read_res == -1) {
			DBG_PRINT("read() failed.\n");
			return -1;
		} else if( read_res == 0) { /* EOF */
			return -1;
		}

		if (buff[count] == '\n' || buff[count] == '\r' || buff[count] == ' '|| buff[count] == '\0') {
			if(count == 0) {
				continue; /* ignore empty command */
			}
			buff[count] = '\0';
			return 0;
		}
		count ++;
	}
	return -1; /* buff was full */
}
