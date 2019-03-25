/*
** Command management
** Sistemas Operativos, Antonio Sarmento, 77906, 2016-2017
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>

#include "command.h"
#include "accounts.h"
#include "logger.h"

/* General material */
#define BUFF_FMT_SIZE 256

/* Thread constants */
#define NUM_THREADS 3
#define CMD_BUFFER_DIM (NUM_THREADS * 2)

command_t cmd_buffer[CMD_BUFFER_DIM];
int buff_write_idx = 0, buff_read_idx = 0;

pthread_mutex_t buffer_ctrl, pending_requests_ctrl;
sem_t *sem_read_ctrl  = NULL,
	  *sem_write_ctrl = NULL;

pthread_t thread_id[NUM_THREADS];

pthread_cond_t cond_simulation;
int pending_requests = 0;

int t_num[NUM_THREADS];

/******************************** Private ***********************************/
void wait_on_read_sem(void) {
	while (sem_wait(sem_read_ctrl) != 0) {
		if (errno == EINTR) {
			continue;
		}

		perror("Error waiting at semaphore \"sem_read_ctrl\"");
		exit(EXIT_FAILURE);
	}
}
void post_to_read_sem(void) {
	if (sem_post(sem_read_ctrl) != 0) {
		perror("Error posting at semaphore \"sem_read_ctrl\"");
		exit(EXIT_FAILURE);
	}
}

void wait_on_write_sem(void) {
	if (sem_wait(sem_write_ctrl) != 0) {
		perror("Error waiting at semaphore \"sem_write_ctrl\"");
		exit(EXIT_FAILURE);
	}
}
void post_to_write_sem(void) {
	if (sem_post(sem_write_ctrl) != 0) {
		perror("Error posting at semaphore \"sem_write_ctrl\"");
		exit(EXIT_FAILURE);
	}
}

void lock_cmd_buffer(void) {
	if ((errno = pthread_mutex_lock(&buffer_ctrl)) != 0) {
		perror("Error in pthread_mutex_lock()");
		exit(EXIT_FAILURE);
	}
}
void unlock_cmd_buffer(void) {
	if ((errno = pthread_mutex_unlock(&buffer_ctrl)) != 0) {
		perror("Error in pthread_mutex_unlock()");
		exit(EXIT_FAILURE);
	}
}

void lock_pending_requests(void) {
	if ((errno = pthread_mutex_lock(&pending_requests_ctrl)) != 0) {
		perror("Error in pthread_mutex_lock()");
		exit(EXIT_FAILURE);
	}
}
void unlock_pending_requests(void) {
	if ((errno = pthread_mutex_unlock(&pending_requests_ctrl)) != 0) {
		perror("Error in pthread_mutex_unlock()");
		exit(EXIT_FAILURE);
	}
}

void wait_cond_simulate() {
	if ((errno = pthread_cond_wait(&cond_simulation, &pending_requests_ctrl)) != 0) {
		perror("Error in pthread_cond_wait()");
		exit(EXIT_FAILURE);
	}
}
void signal_cond_simulate() {
	if ((errno = pthread_cond_signal(&cond_simulation)) != 0) {
		perror("Error in pthread_cond_signal()");
		exit(EXIT_FAILURE);
	}
}

void put_cmd(command_t *cmd) {
	cmd_buffer[buff_write_idx] = *cmd;
	buff_write_idx = (buff_write_idx+1) % CMD_BUFFER_DIM;
}
void get_cmd(command_t *cmd) {
	*cmd = cmd_buffer[buff_read_idx];
	buff_read_idx = (buff_read_idx+1) % CMD_BUFFER_DIM;
}

void *thread_main(void *arg_ptr) {
	int t_num;
	command_t cmd;
	char fmt[BUFF_FMT_SIZE];

	t_num = *((int *)arg_ptr);

	while (1) {
		wait_on_read_sem();
		lock_cmd_buffer();
		get_cmd(&cmd);
		unlock_cmd_buffer();
		post_to_write_sem();

		if (cmd.operation == OP_GET_BALANCE) {
			snprintf(fmt, BUFF_FMT_SIZE, "%s(%d)", op_to_cmd_name(&cmd), cmd.src_account_id);
			int balance = get_balance(cmd.src_account_id);
			if (balance < 0) {
				printf("Erro ao ler saldo da conta %d.\n", cmd.src_account_id);
			} else {
				printf("%s: O saldo da conta é %d.\n\n", fmt, balance);
			}
		}

		else if (cmd.operation == OP_CREDIT) {
			snprintf(fmt, BUFF_FMT_SIZE, "%s(%d, %d)", op_to_cmd_name(&cmd), cmd.src_account_id, cmd.amount);
			if (credit(cmd.src_account_id, cmd.amount) < 0) {
				printf("Erro ao creditar %d na conta %d.\n", cmd.amount, cmd.src_account_id);
			} else {
				printf("%s: OK\n\n", fmt);
			}
		}

		else if (cmd.operation == OP_DEBIT) {
			snprintf(fmt, BUFF_FMT_SIZE, "%s(%d, %d)", op_to_cmd_name(&cmd), cmd.src_account_id, cmd.amount);
			if (debit(cmd.src_account_id, cmd.amount) < 0) {
				printf("Erro ao debitar %d na conta %d.\n", cmd.amount, cmd.src_account_id);
			} else {
				printf("%s: OK\n\n", fmt);
			}
		}

		else if (cmd.operation == OP_TRANSFER) {
			snprintf(fmt, BUFF_FMT_SIZE, "%s(%d, %d, %d)", op_to_cmd_name(&cmd), cmd.src_account_id, cmd.dest_account_id, cmd.amount);
			if (transfer(cmd.dest_account_id, cmd.src_account_id, cmd.amount) < 0) {
				printf("Erro ao transferir %d da conta %d para a conta %d.\n", cmd.amount, cmd.src_account_id, cmd.dest_account_id);
			} else {
				printf("%s: OK\n\n", fmt);
			}
		}

		else if (cmd.operation == OP_EXIT) {
			return NULL;
		}

		else { /* unknown command; ignore */
			printf("Thread %d: Unknown command: %d\n", t_num, cmd.operation);
			continue;
		}

		lock_pending_requests();
		pending_requests--;
		if (!(pending_requests > 0)) {
			signal_cond_simulate();
		}
		unlock_pending_requests();

		write_to_log((unsigned long)pthread_self(), fmt);
	}
}

/******************************** Public ************************************/
/* Constructor, junction and destructor of threads */
void init_threads(void)
{
	int i;
	/* Initializing mutexes */
	if ((errno = pthread_mutex_init(&buffer_ctrl, NULL)) != 0) {
		perror("Could not initialize mutex \"buffer_ctrl\"");
		exit(EXIT_FAILURE);
	}
	if ((errno = pthread_mutex_init(&pending_requests_ctrl, NULL)) != 0) {
		perror("Could not initialize mutex \"pending_requests_ctrl\"");
		exit(EXIT_FAILURE);
	}

	/* Initializing thread condition */
	if ((errno = pthread_cond_init(&cond_simulation, NULL)) != 0) {
		perror("Could not initialize condition \"cond_simulation\"");
		exit(EXIT_FAILURE);
	}

	/* Initializing semaphores */
	sem_read_ctrl = sem_open("sem_read_ctrl",  O_CREAT, 0644, 0);
	if (sem_read_ctrl == SEM_FAILED) {
		perror("Could not initialize semaphore \"sem_read_ctrl\"");
		exit(EXIT_FAILURE);
	}

	sem_write_ctrl = sem_open("sem_write_ctrl",  O_CREAT, 0644, CMD_BUFFER_DIM);
	if (sem_write_ctrl == SEM_FAILED) {
		perror("Could not initialize semaphore \"sem_write_ctrl\"");
		exit(EXIT_FAILURE);
	}

	/* Creating threads */
	for (i = 0; i < NUM_THREADS; ++i) {
		t_num[i] = i;
		if ((errno = pthread_create(&thread_id[i], NULL, thread_main, (void *)&t_num[i])) != 0) {
			perror("Error creating thread");
			exit(EXIT_FAILURE);
		}
	}
}

void join_threads(void)
{
	int i;
	command_t cmd;
	cmd.operation = OP_EXIT;

	/* Tells all threads to exit */
	for (i = 0; i < NUM_THREADS; ++i) {
		insert_cmd(&cmd, false);
	}

	/* Waits for the junction of all threads */
	for (i = 0; i < NUM_THREADS; ++i) {
		if ((errno = pthread_join(thread_id[i], NULL)) != 0) {
			perror("Error joining thread.");
			exit(EXIT_FAILURE);
		}
	}
}

void destroy_threads(void)
{
	sem_close(sem_read_ctrl);
	sem_close(sem_write_ctrl);
	pthread_mutex_destroy(&buffer_ctrl);
	pthread_mutex_destroy(&pending_requests_ctrl);
	pthread_cond_destroy(&cond_simulation);
}

/* Function that waits for pending simulations */
void wait_for_simulation(void)
{
	lock_pending_requests();
	while (pending_requests > 0) {
		wait_cond_simulate();
	}
	unlock_pending_requests();
}

/* Function that fills a command_t */
void new_cmd(command_t *cmd, int operation, int dest_account_id, int src_account_id, int amount)
{
	cmd->operation       = operation;
	cmd->src_account_id  = src_account_id;
	cmd->dest_account_id = dest_account_id;
	cmd->amount          = amount;
}

/* Function that pushes a command to thread (upon request) */
void insert_cmd(command_t *cmd, bool add_to_pending)
{
	wait_on_write_sem();
	put_cmd(cmd);
	post_to_read_sem();

	if (add_to_pending) {
		lock_pending_requests();
		pending_requests++;
		unlock_pending_requests();
	}
}

/* Helpers */
const char *op_to_cmd_name(command_t *cmd)
{
	switch (cmd->operation) {
	case OP_DEBIT:
		return CMD_DEBIT;
	case OP_CREDIT:
		return CMD_CREDIT;
	case OP_TRANSFER:
		return CMD_TRANSFER;
	case OP_GET_BALANCE:
		return CMD_GET_BALANCE;
	case OP_EXIT:
		return CMD_EXIT;
	}

	return NULL;
}
