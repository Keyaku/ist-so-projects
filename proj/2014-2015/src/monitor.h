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

#ifndef __MONITOR_H__
    #define __MONITOR_H__
    
    #define WRITER_PATH "./writer_parallel"
    #define READER_PATH "./reader_parallel"
    
    #define INPUT_BUFFER_SIZE 512
    
    #define EXIT_COMMAND    "sair"
    #define SIGUSR1_COMMAND "il"
    #define SIGUSR2_COMMAND "ie"
    
    #define LINE_FEED		"\n"
    #define LINE_FEED_LEN	(1)
    
    int run_process(char* prog_path, int* stdin_pipe_fd);
    
    void exit_monitor();
    
    int process_command(char* command);

#endif