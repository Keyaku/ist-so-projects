/*
// Projeto SO - exercicio 1, version 03
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "matrix2d.h"

#define ARRAY_LEN(arr) sizeof arr / sizeof *arr


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
| Function: "escrava"
|
| 1. Alocar e receber fatia enviada pela mestre
| 2. Duplicar a fatia (i.e. criar fatia "auxiliar")
| 3. Ciclo das iterações:
|	3.1 Iterar pontos da fatia de calcular temperaturas
|	3.2 Enviar/receber linhas adjacentes
| 4. Enviar fatia calculada para a tarefa mestre
|
---------------------------------------------------------------------*/


/*--------------------------------------------------------------------
| Function: simul
---------------------------------------------------------------------*/
DoubleMatrix2D *simul(
	DoubleMatrix2D *matrix,
	DoubleMatrix2D *matrix_aux,
	int linhas,
	int colunas,
	int iteracoes
) {
	DoubleMatrix2D *result = matrix;
	DoubleMatrix2D *matrix_temp = NULL;

	while (iteracoes > 0) {
		iteracoes -= 1;

		for (int i = 1; i < linhas-1; i++) {
			// FIXME: Criar thread que calcula a linha
			// TODO: Ter em atenção os valores de cima e em baixo (que fazem parte de outra thread)
			for (int j = 1; j < colunas-1; j++) {
				double arr[] = {
					dm2dGetEntry(matrix, i-1, j),
					dm2dGetEntry(matrix, i,   j-1),
					dm2dGetEntry(matrix, i+1, j),
					dm2dGetEntry(matrix, i,   j+1)
				};
				double value = average(arr, 4);
				dm2dSetEntry(matrix_aux, i, j, value);
			}
		}

		/* Switching pointers between matrices, avoids boilerplate code */
		matrix_temp = matrix;
		matrix = matrix_aux;
		matrix_aux = matrix_temp;
	}

	return result;
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
|
| 1. Depois de inicializar a matriz, criar tarefas trabalhadoras
| 2. Enviar fatias para cada tarefa trabalhadora
| 3. Receber fatias calculadas de cada tarefa trabalhadora
| 4. Esperar pela terminação das threads
| 5. Imprimir resultado e libertar memória (usar valgrind)
|
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
	int it = parse_integer_or_exit(argv[6], "iteracoes");

	DoubleMatrix2D *matrix, *matrix_aux, *result;


	fprintf(stderr, "\nArgumentos:\n"
		"    N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d\n",
		N, tEsq, tSup, tDir, tInf, it
	);


	/* VERIFICAR SE ARGUMENTOS ESTAO CONFORME O ENUNCIADO */
	struct {
		double arg, val;
		const char* name;
	} arg_checker[] = {
		{ N,    1, "N"         },
		{ it,   1, "iteracoes" },
		{ tEsq, 0, "tEsq"      },
		{ tSup, 0, "tSup"      },
		{ tDir, 0, "tDir"      },
		{ tInf, 0, "tInf"      }
	};
	for (size_t i = 0; i < ARRAY_LEN(arg_checker); i++) {
		is_arg_greater_equal_to(arg_checker[i].arg, arg_checker[i].val, arg_checker[i].name);
	}

	matrix = dm2dNew(N+2, N+2);
	matrix_aux = dm2dNew(N+2, N+2);

	/* FIXME: FAZER ALTERACOES AQUI */
	dm2dSetLineTo(matrix, 0, tSup);
	dm2dSetLineTo(matrix, N+1, tInf);
	dm2dSetColumnTo(matrix, 0, tEsq);
	dm2dSetColumnTo(matrix, N+1, tDir);

	dm2dCopy(matrix_aux, matrix);

	result = simul(matrix, matrix_aux, N+2, N+2, it);
	dm2dPrint(result);

	dm2dFree(matrix);
	dm2dFree(matrix_aux);

	return 0;
}
