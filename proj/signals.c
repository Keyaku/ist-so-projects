#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "signals.h"

/* "Private" material */
// General
void link_signal(int signum, void (*handler)(int)) {
	if (signal(signum, handler) == SIG_ERR) {
		fprintf(stderr, "Erro ao definir sinal.");
		exit(EXIT_FAILURE);
	}
}

// SIGINT
int interrupted;
void manage_interrupt(int signum) {
	if (signum != SIGINT) {
		fprintf(stderr, "Signal other than SIGINT was deployed.\n");
		exit(EXIT_FAILURE);
	}

	// FIXME: reset SIGINT necessary?
	interrupted = 1;
}

// SIGALRM
// TODO

/* Public functions */
void signals_init() {
	/* Inicializar o sinal de interrupção SIGINT */
	interrupted = 0;
	link_signal(SIGINT, manage_interrupt);

	/* Inicializar o sinal de alarme SIGALRM */
	// TODO
}

int signals_was_interrupted() {
	return interrupted;
}
