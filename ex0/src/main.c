/*
// Projeto SO - exercicio 1, version 03
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "matrix2d.h"


/*--------------------------------------------------------------------
| Function: average
---------------------------------------------------------------------*/
double average(double *array, size_t size) {
	double result = 0;

	for (size_t idx = 0; idx < size; idx++) {
		result += array[idx];
	}

	return result / size;
}

/*--------------------------------------------------------------------
| Function: simul
---------------------------------------------------------------------*/
DoubleMatrix2D *simul(
	DoubleMatrix2D *matrix,
	DoubleMatrix2D *matrix_aux,
	int linhas,
	int colunas,
	int numIteracoes
) {
	for (int i = 0; i < linhas; i++) {
		for (int j = 0; j < colunas; j++) {
			double arr[] = {
				dm2dGetEntry(matrix, i-1, j),
				dm2dGetEntry(matrix, i+1, j),
				dm2dGetEntry(matrix, i, j-1),
				dm2dGetEntry(matrix, i, j+1)
			};
			double value = average(arr, 4);

			// FIXME: use both matrices to get results
			dm2dSetEntry(matrix_aux, i, j, value);
		}
	}
	// FIXME: return the resulting matrix
	return matrix_aux;
}

/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
---------------------------------------------------------------------*/
int parse_integer_or_exit(const char *str, const char *name) {
	int value;

	if (sscanf(str, "%d", &value) != 1) {
		fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
		exit(1);
	}
	return value;
}

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
---------------------------------------------------------------------*/
double parse_double_or_exit(const char *str, const char *name) {
	double value;

	if (sscanf(str, "%lf", &value) != 1) {
		fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
		exit(1);
	}
	return value;
}

/*--------------------------------------------------------------------
| Function: is_arg_greater_equal_to
---------------------------------------------------------------------*/
void is_arg_greater_equal_to(int value, int greater, const char *name) {
	if (value < greater) {
		fprintf(stderr, "\n%s tem que ser >= %d\n\n", name, greater);
		exit(1);
	}
}

/*--------------------------------------------------------------------
| Function: main
---------------------------------------------------------------------*/
int main (int argc, char *argv[]) {
	if (argc != 7) {
		fprintf(stderr, "\nNumero invalido de argumentos.\n");
		fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iteracoes\n\n");
		return 1;
	}

	/* argv[0] = program name */
	int N = parse_integer_or_exit(argv[1], "N");
	double tEsq = parse_double_or_exit(argv[2], "tEsq");
	double tSup = parse_double_or_exit(argv[3], "tSup");
	double tDir = parse_double_or_exit(argv[4], "tDir");
	double tInf = parse_double_or_exit(argv[5], "tInf");
	int iteracoes = parse_integer_or_exit(argv[6], "iteracoes");

	DoubleMatrix2D *matrix, *matrix_aux, *result;


	fprintf(stderr, "\nArgumentos:\n"
	" N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d\n",
	N, tEsq, tSup, tDir, tInf, iteracoes);


	/* VERIFICAR SE ARGUMENTOS ESTAO CONFORME O ENUNCIADO */
	is_arg_greater_equal_to(N, 1, "N");
	is_arg_greater_equal_to(tEsq, 0, "tEsq");
	is_arg_greater_equal_to(tSup, 0, "tSup");
	is_arg_greater_equal_to(tDir, 0, "tDir");
	is_arg_greater_equal_to(tInf, 0, "tInf");
	is_arg_greater_equal_to(iteracoes, 1, "iteracoes");

	matrix = dm2dNew(N+2, N+2);
	matrix_aux = dm2dNew(N+2, N+2);


	/* FIXME: FAZER ALTERACOES AQUI */


	dm2dSetLineTo(matrix, 0, tSup);
	dm2dSetLineTo(matrix, N+1, tInf);
	dm2dSetColumnTo(matrix, 0, tEsq);
	dm2dSetColumnTo(matrix, N+1, tDir);

	dm2dCopy(matrix_aux, matrix);

	result = simul(matrix, matrix_aux, N+2, N+2, iteracoes);
	dm2dPrint(result);

	dm2dFree(matrix);
	dm2dFree(matrix_aux);

	return 0;
}
