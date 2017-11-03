/*
// Bibliotece de matrizes 2D alocadas dinamicamente, versao 3
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include "matrix2d.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*--------------------------------------------------------------------
| Function: dm2dNew
---------------------------------------------------------------------*/

DoubleMatrix2D *dm2dNew(int lines, int columns) {
	int i, j;
	DoubleMatrix2D *matrix = malloc(sizeof(*matrix));

	if (matrix == NULL) {
		return NULL;
	}

	matrix->n_l = lines;
	matrix->n_c = columns;
	matrix->data = malloc(lines*columns * sizeof(*matrix->data));
	if (matrix->data == NULL) {
		free(matrix);
		return NULL;
	}

	for (i = 0; i < lines; i++) {
		for (j = 0; j < columns; j++) {
			dm2dSetEntry(matrix, i, j, 0);
		}
	}

	return matrix;
}

/*--------------------------------------------------------------------
| Function: dm2dFree
---------------------------------------------------------------------*/

void dm2dFree(DoubleMatrix2D *matrix) {
	free(matrix->data);
	free(matrix);
}

/*--------------------------------------------------------------------
| Function: dm2dGetLine
---------------------------------------------------------------------*/

double* dm2dGetLine(DoubleMatrix2D *matrix, int line_nb) {
	return &(matrix->data[line_nb*matrix->n_c]);
}

/*--------------------------------------------------------------------
| Function: dm2dSetLine
---------------------------------------------------------------------*/

void dm2dSetLine(DoubleMatrix2D *matrix, int line_nb, double* line_values) {
	memcpy((char*) &(matrix->data[line_nb*matrix->n_c]), line_values, matrix->n_c*sizeof(double));
}

/*--------------------------------------------------------------------
| Function: dm2dSetLineTo
---------------------------------------------------------------------*/

void dm2dSetLineTo(DoubleMatrix2D *matrix, int line, double value) {
	int i;

	for (i = 0; i < matrix->n_c; i++) {
		dm2dSetEntry(matrix, line, i, value);
	}
}


/*--------------------------------------------------------------------
| Function: dm2dSetColumnTo
---------------------------------------------------------------------*/

void dm2dSetColumnTo(DoubleMatrix2D *matrix, int column, double value) {
	int i;

	for (i = 0; i < matrix->n_l; i++) {
		dm2dSetEntry(matrix, i, column, value);
	}
}

/*--------------------------------------------------------------------
| Function: dm2dCopy
---------------------------------------------------------------------*/

void dm2dCopy(DoubleMatrix2D *to, DoubleMatrix2D *from) {
	memcpy(to->data, from->data, sizeof(double)*to->n_l*to->n_c);
}


/*--------------------------------------------------------------------
| Function: dm2dPrint
---------------------------------------------------------------------*/

void dm2dPrint(DoubleMatrix2D *matrix) {
	int i, j;

	printf ("\n");
	for (i = 0; i < matrix->n_l; i++) {
		for (j = 0; j < matrix->n_c; j++) {
			printf(" %8.4f", dm2dGetEntry(matrix, i, j));
		}
		printf ("\n");
	}
}

/*--------------------------------------------------------------------
| Function: dm2dDelimited
| Checks diagonally in the shape of an X for delimiter
---------------------------------------------------------------------*/
#define max(a, b) a < b ? b : a

int dm2dDelimited(DoubleMatrix2D *m, DoubleMatrix2D *m_aux, int n, double delimiter) {
	int half = (n >> 1)-1;
	n--;
	double d = 0;

	if (delimiter > 0) {
		for (int idx = 1; idx < n; idx++) {
			double diff = dm2dGetEntry(m_aux, idx, half) - dm2dGetEntry(m, idx, half);
			d = max(d, diff);
			if (d > delimiter) { return 0; }

			diff = dm2dGetEntry(m_aux, half, idx) - dm2dGetEntry(m, half, idx);
			d = max(d, diff);
			if (d > delimiter) { return 0; }
		}
	}

	return d < delimiter;
}

#undef max
