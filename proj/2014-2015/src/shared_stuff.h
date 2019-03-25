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

#ifndef __SHARED_STUFF_H__
	#define __SHARED_STUFF_H__

	#include <stdio.h>

	/* enables debug prints if defined; we define it in the makefile
	#define DEBUG
	*/
	/* don't use colors for debug -- meh
	#define DEBUG_NO_COLORS
	*/

	/* terminal colors for DEBUG_COLORS */
	#define KNRM  "\x1B[0m"
	#define KRED  "\x1B[31m"
	#define KGRN  "\x1B[32m"
	#define KYEL  "\x1B[33m"
	#define KBLU  "\x1B[34m"
	#define KMAG  "\x1B[35m"
	#define KCYN  "\x1B[36m"
	#define KWHT  "\x1B[37m"
	#define KGRY  "\x1B[90m"

	#define WRITER_STRING_LEN   10
	#define WRITER_STRING_COUNT 10
	#define LINES_PER_FILE      1024

	#define FILENAME		"SO2014-%d.txt"
	#define INPUT_LEN		100

	#define RANDOM_RANGE(MIN, MAX) (( rand() % ( MAX + 1 - MIN ) ) + MIN )

	/* used to print debug messages */
	#ifdef DEBUG
		#ifndef DEBUG_NO_COLORS
			#define DBG_PRINTF(format, ...) printf(KYEL"%s"KGRY":"KBLU"%d"KGRY": " KRED format KNRM, __FILE__, __LINE__, __VA_ARGS__)
			#define DBG_PRINT(format) printf(KYEL"%s"KGRY":"KBLU"%d"KGRY": " KGRN format KNRM, __FILE__, __LINE__)
		#else
			#define DBG_PRINTF(format, ...) printf("%s:%d: "format, __FILE__, __LINE__, __VA_ARGS__)
			#define DBG_PRINT(format) printf("%s:%d: "format, __FILE__, __LINE__)
		#endif
	#else
	    #define DBG_PRINTF(format, ...)
	    #define DBG_PRINT(format)
	#endif

	#define TRUE    1
	#define FALSE   0

	extern char writer_strings[ WRITER_STRING_COUNT ][ WRITER_STRING_LEN + 1 ];

	int read_command_from_fd(int fd, char * buff, size_t buff_size);

#endif
