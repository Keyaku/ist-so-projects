/*
** Main project file
** Sistemas Operativos, Antonio Sarmento, 77906, 2016-2017
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "commandlinereader.h"
#include "accounts.h"
#include "command.h"
#include "logger.h"

/* Useful constants */
#define MAX_ARGS 4
#define MAX_CHILDREN 20
#define SIZE_BUFFER 100

#define PROGNAME "i-banco"

int main(int argc, char* argv[])
{
    char *args[MAX_ARGS + 1];
    char buffer[SIZE_BUFFER];
	bool keep_looping = true,
		exit_now = false;

	/* Process data */
	int num_children = 0;
	pid_t pid_children[MAX_CHILDREN];

	/* Command data */
	command_t cmd;
	bool push_to_thread;

	/* Assign SIGUSR1 to manage_signal */
	if (signal(SIGUSR1, manage_signal) == SIG_ERR) {
		perror("Error defining signal.");
		exit(EXIT_FAILURE);
    }

    init_accounts();
	init_threads();
	init_logger();

    printf("Bem-vinda/o ao i-banco\n\n");

    while (keep_looping) {
		/* Initialize data for beginning of loop */
        int numargs = readLineArguments(args, MAX_ARGS+1, buffer, SIZE_BUFFER);
		push_to_thread = false;

        /* EOF (end of file) from stdin, or CMD_EXIT */
        if (numargs < 0 || (numargs > 0 && (strcmp(args[0], CMD_EXIT) == 0))) {
			keep_looping = false;
			if (numargs > 1 && strcmp(args[1], CMD_EXIT_ARG1) == 0) {
				exit_now = true;
			}
			continue;
        }

		/* No argument; ignore and ask again */
        else if (numargs == 0) {
            continue;
		}

        /* Debit */
        else if (strcmp(args[0], CMD_DEBIT) == 0) {
            if (numargs < 3) {
            	printf("%s: Sintaxe inválida, tente de novo.\n", CMD_DEBIT);
	        	continue;
            }
			new_cmd(&cmd, OP_DEBIT, 0, atoi(args[1]), atoi(args[2]));
			push_to_thread = true;
    	}

	    /* Credit */
	    else if (strcmp(args[0], CMD_CREDIT) == 0) {
	        if (numargs < 3) {
	            printf("%s: Sintaxe inválida, tente de novo.\n", CMD_CREDIT);
	            continue;
	        }
			new_cmd(&cmd, OP_CREDIT, 0, atoi(args[1]), atoi(args[2]));
			push_to_thread = true;
	    }

		/* Transfer */
		else if (strcmp(args[0], CMD_TRANSFER) == 0) {
			if (numargs < 4) {
				printf("%s: Sintaxe inválida, tente de novo.\n", CMD_TRANSFER);
				continue;
			}
			new_cmd(&cmd, OP_TRANSFER, atoi(args[2]), atoi(args[1]), atoi(args[3]));
			push_to_thread = true;
		}

	    /* Get Balance */
	    else if (strcmp(args[0], CMD_GET_BALANCE) == 0) {
	        if (numargs < 2) {
	            printf("%s: Sintaxe inválida, tente de novo.\n", CMD_GET_BALANCE);
	            continue;
	        }
			new_cmd(&cmd, OP_GET_BALANCE, 0, atoi(args[1]), 0);
			push_to_thread = true;
	    }

	    /* Simulate */
	    else if (strcmp(args[0], CMD_SIMULATE) == 0) {
			int num_years;
			pid_t pid;

			if (num_children >= MAX_CHILDREN) {
				printf("%s: Atingido o número máximo de processos filho a criar.\n", CMD_SIMULATE);
				continue;
			}
			if (numargs < 2) {
				printf("%s: Sintaxe inválida, tente de novo.\n", CMD_SIMULATE);
				continue;
			}

			num_years = atoi(args[1]);
			wait_for_simulation();

			pid = fork();
			if (pid < 0) {
				perror("Failed to create new process.");
				exit(EXIT_FAILURE);
			} else if (pid > 0) {
				pid_children[num_children] = pid;
				num_children++;
				printf("%s(%d): Simulação iniciada em background.\n\n", CMD_SIMULATE, num_years);
				continue;
			} else {
				simulate(num_years);
				exit(EXIT_SUCCESS);
			}
	    }

		/* Default */
		else {
			printf("Comando desconhecido. Tente de novo.\n");
		}

		if (push_to_thread) {
			insert_cmd(&cmd, true);
		}
	}

	/* If we've reached this far, shut everything down */
	printf("%s vai terminar.\n--\n", PROGNAME);

	/* Kill all children in case CMD_EXIT_ARG1 was given */
	if (exit_now) {
		int i;
		for (i = 0; i < num_children; i++) {
			kill(pid_children[num_children], SIGUSR1);
		}
	}

	/* Join all threads */
	join_threads();

	/* Waiting for every child process */
	while (num_children > 0) {
		int status;
		pid_t childpid = wait(&status);

		if (childpid < 0) {
			if (errno == EINTR) {
				/* This indicates the interruption of our child with a signal;
				** we shall then ignore this error and keep waiting
				*/
				continue;
			} else if (errno == ECHILD) {
				/* This means that there are no more children left to wait for */
				break;
			} else {
				perror("Unexpected error while waiting for child.");
				exit(EXIT_FAILURE);
			}
		}

		if (WIFEXITED(status)) {
			printf("FILHO TERMINADO (PID=%d; terminou normalmente)\n",
				childpid);
		} else {
			printf("FILHO TERMINADO (PID=%d; terminou abruptamente)\n",
				childpid);
		}
		num_children--;
	}

	printf("--\n%s terminou.\n", PROGNAME);

	destroy_threads();
	destroy_accounts();
	destroy_logger();

	exit(EXIT_SUCCESS);
}
