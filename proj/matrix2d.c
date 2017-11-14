/*
// Biblioteca de matrizes 2D alocadas dinamicamente, versao 4
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
	for (int i = 0; i < matrix->n_c; i++) {
		dm2dSetEntry(matrix, line, i, value);
	}
}


/*--------------------------------------------------------------------
| Function: dm2dSetColumnTo
---------------------------------------------------------------------*/

void dm2dSetColumnTo(DoubleMatrix2D *matrix, int column, double value) {
	for (int i = 0; i < matrix->n_l; i++) {
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
| Function: dm2dPrintStream & dm2dPrint
---------------------------------------------------------------------*/

void dm2dPrintStream(FILE *stream, DoubleMatrix2D *matrix) {
	for (int i = 0; i < matrix->n_l; i++) {
		for (int j = 0; j < matrix->n_c; j++) {
			fprintf(stream, " %8.4f", dm2dGetEntry(matrix, i, j));
		}
		fprintf(stream, "\n");
	}
}

void dm2dPrint(DoubleMatrix2D *matrix) {
	printf("\n");
	dm2dPrintStream(stdout, matrix);
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

/*--------------------------------------------------------------------
| Function: readMatrix2dFromFile & writeMatrix2dToFile
---------------------------------------------------------------------*/
DoubleMatrix2D *readMatrix2dFromFile(FILE *f, int l, int c) {
	double v;
	DoubleMatrix2D *m;

	if (f == NULL || l < 1 || c < 1) {
		return NULL;
	}

	m = dm2dNew(l, c);
	if (m == NULL) {
		return NULL;
	}

	// Ler pontos da matriz
	// Nesta implementacao, ignora a existencia e posicionamento
	// de quebras de linha
	for (int i = 0; i < l; i++) {
		for (int j = 0; j < c; j++) {
			if (fscanf(f, "%lf", &v) != 1) {
				dm2dFree(m);
				return NULL;
			}
			dm2dSetEntry(m, i, j, v);
		}
	}

	return m;
}

void writeMatrix2dToFile(const char *filename, DoubleMatrix2D *matrix) {
	/* Discarding old file and saving new matrix */
	FILE *f = fopen(filename, "w");
	if (f == NULL) {
		fprintf(stderr, "Não foi possível escrever sobre \"%s\"\n", filename);
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "Writing to file...\n"); // FIXME: remove this line
	dm2dPrintStream(f, matrix);

	/* Closing stream */
	if (fclose(f)) {
		fprintf(stderr, "Não foi possível fechar o ficheiro\"%s\"\n", filename);
		exit(EXIT_FAILURE);
	}
}
