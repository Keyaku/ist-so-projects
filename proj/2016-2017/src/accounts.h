/*
** Operations on accounts
** Sistemas Operativos, Antonio Sarmento, 77906, 2016-2017
*/

#ifndef ACCOUNTS_H
#define ACCOUNTS_H


/* Public methods */
void init_accounts(void);
void destroy_accounts(void);

int debit(int account_id, int amount);
int credit(int account_id, int amount);
int transfer(int dest_account_id, int src_account_id, int amount);
int get_balance(int account_id);
void manage_signal(int signum);
void simulate(int);


#endif /* ACCOUNTS_H */
