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
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>



#include "writer.h"
#include "shared_stuff.h"


static const int writer_f_flags = O_WRONLY | O_CREAT;
static const int writer_f_mode  = S_IRUSR  | S_IWUSR | S_IROTH;

void writer(int file_num, char *txt, int txt_len, int use_locks, int enable_writing_errors) {
	char file_name[64];
	int fd, i, txt_size;

	/* wrong/correct writer vars below */
	int line_counter = 0; /* if it's even, don't introduce an error, else, do */
    int str_index = 0;
    char* wrong_str = NULL;
    char* str_to_write = NULL; /* points to the string that will actually be written */

	sprintf(file_name, FILENAME, file_num);	/* place file_num in FILENAME */
	fd = open(file_name, writer_f_flags, writer_f_mode);
	if (fd == -1) {
		printf("open(%s) failed. errno=%d\n", file_name, errno);
	}
	
	if(use_locks) {
		flock(fd, LOCK_EX);
	}

	txt_size = txt_len * sizeof(char);

	if (enable_writing_errors) {
		wrong_str = (char*) malloc( txt_size + sizeof(char) );
	}

	for (i = 0; i < LINES_PER_FILE; i++){
		str_to_write = txt; /* by default, write the correct string */

		if(enable_writing_errors) {
			if (line_counter % 2) {
	            /* line is odd - introduce an error in that line */
	            strcpy(wrong_str, txt);
	            str_index = RANDOM_RANGE(0, txt_len - 1);
	            /* change a character's offset on a random position to a random offset */
	            wrong_str[str_index] = wrong_str[str_index] +
	                                    	RANDOM_RANGE(MIN_OFFSET, MAX_OFFSET);
	            str_to_write = wrong_str;
	        }

	       line_counter++; /* only needed when writing errors */
        }

		write(fd, str_to_write, txt_size);
	}

	if(use_locks) {
		flock(fd, LOCK_UN);
	}

	if (enable_writing_errors && wrong_str != NULL)
		free(wrong_str);

	close(fd);
}


char *get_writer_string(int index){
	return (char*) writer_strings[index];
}

