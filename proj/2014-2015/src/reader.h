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

#ifndef __READER_H__
	#define __READER_H__


	#define FILE_IS_VALID	 0
	#define FILE_IS_INVALID	-1
	
	
	/**
	 * @brief Check the specified file for consistency.
	 *
	 * @param filename The file's name
	 *
	 * @retval FILE_IS_VALID or FILE_IS_INVALID
	 */
	int reader(char *filename);

	/**
	 * @brief Check the specified and limited file for consistency.
	 *
	 * @param filename The file's name
	 * @param first_line Line to start reading
	 * @param last_line Line where reading ends
	 *
	 * @retval FILE_IS_VALID or FILE_IS_INVALID
	 */
	int reader_ranged(char *filename, int first_line, int last_line);

	/**
	 * @brief Checks wether the file is valid.
	 * A file is considered valid if it repeats one of the known writer strings
	 * line_count times.
	 *
	 * @param fd The file descriptor of the file to check
	 * @param line_length Expected length of each line, in characters
	 * @param first_line First line to start checking
	 * @param last_line Last line where checking ends
	 *
	 * @retval TRUE if the file is valid; FALSE otherwise
	 */
	int file_contents_are_valid(int fd, int line_length, int first_line, int last_line);

	/**
	 * @brief looks for str in writer_strings
	 *
	 * @param str The string to look for
	 * @param str_len Number of characters in str
	 *
	 * @retval TRUE if str is in writer_strings; FALSE otherwise
	 */
	int known_writer_string(char *str, int str_len);


#endif
