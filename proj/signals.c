#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "signals.h"

/* "Private" material */

// SIGINT
int interrupted;
void signals_manage_interrupt(int signum) {
	if (signum != SIGINT) {
		fprintf(stderr, "Signal other than SIGINT was deployed.\n");
		exit(EXIT_FAILURE);
	}
	interrupted = 1;
}

/* Public functions */
void signals_init() {
	/* Inicializar o sinal de interrupção */
	interrupted = 0;
	if (signal(SIGINT, signals_manage_interrupt) == SIG_ERR) {
		fprintf(stderr, "Erro ao definir sinal.");
		exit(EXIT_FAILURE);
	}

	/* Inicializar o sinal de alarme */
	// TODO
}

int signals_was_interrupted() {
	return interrupted;
}
