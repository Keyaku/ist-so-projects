#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "helpers.h"

double average(double *array, size_t size) {
	double result = 0;

	for (size_t idx = 0; idx < size; idx++) {
		result += array[idx];
	}

	return result / size;
}

/* Helpers for child process */
void wait_properly(pid_t pid, int flags) {
	int wstatus = 0;
	waitpid(pid, &wstatus, flags);
	if (!WIFEXITED(wstatus)) {
		fprintf(stderr, "Processo filho não terminou correctamente\n");
	}
	// FIXME: add more wstatus checks?
}
