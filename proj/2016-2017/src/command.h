/*
** Command management
** Sistemas Operativos, Antonio Sarmento, 77906, 2016-2017
*/

#ifndef COMMAND_TYPE_H
#define COMMAND_TYPE_H

#include <stdbool.h>

/* Command codes */
enum {
	OP_DEBIT      ,
	OP_CREDIT     ,
	OP_GET_BALANCE,
	OP_EXIT       ,
	OP_TRANSFER
};

/* Command names */
#define CMD_DEBIT       "debitar"
#define CMD_CREDIT      "creditar"
#define CMD_TRANSFER    "transferir"
#define CMD_GET_BALANCE "lerSaldo"
#define CMD_SIMULATE    "simular"
#define CMD_EXIT        "sair"
#define CMD_EXIT_ARG1   "agora"

/* Command type structure */
typedef struct {
	int operation;
	int src_account_id;
	int dest_account_id;
	int amount;
} command_t;

/* Public methods */
void init_threads(void);
void join_threads(void);
void destroy_threads(void);
void wait_for_simulation(void);

void new_cmd(command_t *cmd, int operation, int dest_account_id, int src_account_id, int amount);
void insert_cmd(command_t *cmd, bool add_to_pending);
const char *op_to_cmd_name(command_t *cmd);

#endif /* COMMAND_TYPE_H */
