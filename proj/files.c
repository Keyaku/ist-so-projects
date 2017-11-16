#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "files.h"

/*--------------------------------------------------------------------
| Function: File handling
| - file_exists()
| - file_delete()
---------------------------------------------------------------------*/
int file_exists(const char* filename) {
	FILE *f = fopen(filename, "r");

	if (f != NULL) {
		fclose(f);
		return F_EXISTS;
	}
	if (errno == ENOENT) {
		return F_NOT_EXISTS;
	}
	return F_ERROR;
}

void file_delete(const char *filename) {
	if (filename != NULL) {
		/* In case removing the file breaks */
		if (unlink(filename)) {
			if (errno != ENOENT) {
				fprintf(stderr, "Não foi possível remover \"%s\"\n", filename);
				exit(EXIT_FAILURE);
			}
			/* In any other case, we ignore it */
		}
	}
}
