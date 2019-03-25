/*
** Operations on accounts
** Sistemas Operativos, Antonio Sarmento, 77906, 2016-2017
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "accounts.h"

#define DELAY_AMOUNT 1
#define NUM_ACCOUNTS 10
#define TAX_INTEREST 0.1
#define COST_MAINTENANCE 1

#ifndef SPEEDTEST
	#define delay() sleep(DELAY_AMOUNT)
#else
	#define delay()
#endif
#define max(a, b) a > b ? a : b
#define min(a, b) a > b ? b : a

int issued_kill = 0;
int account_balance[NUM_ACCOUNTS];
pthread_mutex_t account_ctrl[NUM_ACCOUNTS];

/******************************** Private ***********************************/
/* Multi-thread material */
void lock_account(int account) {
	/* confirm first if it was verified that it's a valid account */
	if ((errno = pthread_mutex_lock(&account_ctrl[account-1])) != 0) {
		perror("Error in pthread_mutex_lock()");
		exit(EXIT_FAILURE);
	}
}

void unlock_account(int account) {
	/* confirm first if it was verified that it's a valid account */
	if ((errno = pthread_mutex_unlock(&account_ctrl[account-1])) != 0) {
		perror("Error in pthread_mutex_unlock()");
		exit(EXIT_FAILURE);
	}
}

/* Account-related material */
int account_exists(int account_id) {
	return (account_id > 0 && account_id <= NUM_ACCOUNTS);
}

int debit_no_mutex(int account_id, int amount) {
	delay();
	if (!account_exists(account_id)) {
		return -1;
	}
	if (account_balance[account_id - 1] < amount) {
		return -1;
	}
	delay();
	account_balance[account_id - 1] -= amount;

	return 0;
}

int credit_no_mutex(int account_id, int amount) {
	delay();
	if (!account_exists(account_id)) {
		return -1;
	}
	account_balance[account_id - 1] += amount;
	return 0;
}

/******************************** Public ************************************/
void init_accounts(void)
{
	int i;
	for (i = 0; i < NUM_ACCOUNTS; i++) {
		account_balance[i] = 0;
		if ((errno = pthread_mutex_init(&account_ctrl[i], NULL)) != 0) {
			perror("Error in pthread_mutex_init()");
			exit(EXIT_FAILURE);
		}
	}
}

void destroy_accounts(void)
{
	int i;
	for (i = 0; i < NUM_ACCOUNTS; i++) {
		pthread_mutex_destroy(&account_ctrl[i]);
	}
}

/* Operating on accounts */
int debit(int account_id, int amount)
{
	int i;
	lock_account(account_id);
	i = debit_no_mutex(account_id, amount);
	unlock_account(account_id);
	return i;
}

int credit(int account_id, int amount)
{
	int i;
	lock_account(account_id);
	i = credit_no_mutex(account_id, amount);
	unlock_account(account_id);
	return i;
}

int transfer(int dest_account_id, int src_account_id, int amount)
{
	if (!account_exists(src_account_id) || !account_exists(dest_account_id)) {
		return -1;
	}

	if (src_account_id < dest_account_id) {
		lock_account(src_account_id);
		lock_account(dest_account_id);
	} else if (src_account_id > dest_account_id) {
		lock_account(dest_account_id);
		lock_account(src_account_id);
	} else {
		return -1;
	}

	if (debit_no_mutex(src_account_id, amount) < 0) {
		unlock_account(src_account_id);
		unlock_account(dest_account_id);
		return -1;
	}

	credit_no_mutex(dest_account_id, amount);

	unlock_account(src_account_id);
	unlock_account(dest_account_id);
	return 0;
}

int get_balance(int account_id)
{
	int saldo;

	delay();
	if (!account_exists(account_id)) {
		return -1;
	}

	lock_account(account_id);
	saldo = account_balance[account_id - 1];
	unlock_account(account_id);
	return saldo;
}

void manage_signal(int signum)
{
	issued_kill = 1;
}

void simulate(int num_years)
{
	int year, id;
	for (year = 0; year <= num_years && !issued_kill; year++) {
		printf("SIMULACAO: Ano %d\n=================\n", year);
		for (id = 1; id <= NUM_ACCOUNTS; id++) {
			int balance;

			if (year > 0) {
				balance = get_balance(id);
				credit(id, balance * TAX_INTEREST);
				balance = get_balance(id);
				debit(id, min(COST_MAINTENANCE, balance));
			}
			balance = get_balance(id);

			while (printf("Conta %5d,\t Saldo %10d\n", id, balance) < 0) {
				if (errno == EINTR) {
					continue;
				} else {
					break;
				}
			}
		}
	}

	if (issued_kill) {
		printf("Simulacao terminada por signal\n");
	}
}
