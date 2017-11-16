#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "signals.h"

/*--------------------------------------------------------------------
| Private functions
---------------------------------------------------------------------*/
// General
#define test_signal(sig, a) { \
	if (a != sig) { \
		fprintf(stderr, "Signal other than "#sig" was deployed.\n"); \
		exit(EXIT_FAILURE); \
	} \
}

void link_signal(int signum, void (*handler)(int)) {
	if (signal(signum, handler) == SIG_ERR) {
		fprintf(stderr, "Erro ao definir sinal.");
		exit(EXIT_FAILURE);
	}
}

// SIGINT
int interrupted;
void manage_interrupt(int signum) {
	test_signal(SIGINT, signum);

	// FIXME: reset SIGINT necessary?
	interrupted = 1;
}

// SIGALRM
int alarm_interval;
int alarmed;
void manage_alarm(int signum) {
	test_signal(SIGALRM, signum);
	alarmed = 1;
}

/*--------------------------------------------------------------------
| Public functions
---------------------------------------------------------------------*/
void signals_init(int interval) {
	/* Inicializar o sinal de interrupção SIGINT */
	interrupted = 0;
	link_signal(SIGINT, manage_interrupt);

	/* Inicializar o sinal de alarme SIGALRM */
	if (interval > 0) {
		signals_reset_alarm(interval);
	}
}

int signals_was_interrupted() {
	return interrupted;
}

int signals_was_alarmed() {
	return alarmed;
}

void signals_reset_alarm(int interval) {
	alarmed = 0;
	alarm_interval = interval;
	link_signal(SIGALRM, manage_alarm);
	alarm(interval);
}
