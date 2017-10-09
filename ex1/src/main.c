/*
// Projeto SO - exercicio 1, version 03
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "matrix2d.h"
#include "mplib3.h"

#define ARRAY_LEN(arr) sizeof arr / sizeof *arr

/* Número de linhas a processar por tarefa trabalhadora */
int k;

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
// TODO

/*--------------------------------------------------------------------
| Function: simul
---------------------------------------------------------------------*/
DoubleMatrix2D *simul(
	DoubleMatrix2D *matrix,
	DoubleMatrix2D *matrix_aux,
	int linhas,
	int colunas,
	int iter
) {
	DoubleMatrix2D *result = matrix;
	DoubleMatrix2D *matrix_temp = NULL;

	while (iter > 0) {
		iter -= 1;

		for (int i = 1; i < linhas-1; i++) {
			for (int j = 1; j < colunas-1; j++) {
				double arr[] = {
					dm2dGetEntry(matrix, i-1, j),
					dm2dGetEntry(matrix, i,   j-1),
					dm2dGetEntry(matrix, i+1, j),
					dm2dGetEntry(matrix, i,   j+1)
				};
				double value = average(arr, ARRAY_LEN(arr));
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
| 1. [√] Depois de inicializar a matriz, criar tarefas trabalhadoras
| 2. [ ] Enviar fatias para cada tarefa trabalhadora
| 3. [ ] Receber fatias calculadas de cada tarefa trabalhadora
| 4. [ ] Esperar pela terminação das threads
| 5. [ ] Imprimir resultado e libertar memória (usar valgrind)
|
---------------------------------------------------------------------*/
int main (int argc, char *argv[]) {
	if (argc != 7 && argc != 9) {
		fprintf(stderr, "\nNumero invalido de argumentos.\n");
		fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iter trab csz\n\n");
		return 1;
	}

	/* argv[0] = program name */
	int N    = parse_integer_or_exit(argv[1], "N");
	int iter = parse_integer_or_exit(argv[6], "iter");
	int trab = 1, csz = 1;
	if (8 <= argc && argc <= 9) {
		trab = parse_integer_or_exit(argv[7], "trab");
		csz  = parse_integer_or_exit(argv[8], "csz");
	}

	struct { double esq, sup, dir, inf; } t;
	t.esq = parse_double_or_exit(argv[2], "tEsq");
	t.sup = parse_double_or_exit(argv[3], "tSup");
	t.dir = parse_double_or_exit(argv[4], "tDir");
	t.inf = parse_double_or_exit(argv[5], "tInf");

	DoubleMatrix2D *matrix, *matrix_aux, *result;


	fprintf(stderr, "\nArgumentos:\n"
		"    N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d trab=%d csz=%d\n",
		N, t.esq, t.sup, t.dir, t.inf, iter, trab, csz
	);


	/* VERIFICAR SE ARGUMENTOS ESTÃO CONFORME O ENUNCIADO */
	struct {
		double arg, val;
		const char* name;
	} arg_checker[] = {
		{ N,     1, "N"    },
		{ t.esq, 0, "tEsq" },
		{ t.sup, 0, "tSup" },
		{ t.dir, 0, "tDir" },
		{ t.inf, 0, "tInf" },
		{ iter,  1, "iter" },
		{ trab,  1, "trab" },
		{ csz,   1, "csz"  }
	};
	for (size_t idx = 0; idx < ARRAY_LEN(arg_checker); idx++) {
		is_arg_greater_equal_to(arg_checker[idx].arg, arg_checker[idx].val, arg_checker[idx].name);
	}

	k = N/trab;
	if ((double) k != (double) N/trab) {
		fprintf(stderr, "\ntrab não é um múltiplo de N\n");
		exit(1);
	}

	/* Criar matrizes */
	matrix = dm2dNew(N+2, N+2);
	matrix_aux = dm2dNew(N+2, N+2);

	/* FAZER ALTERACOES AQUI */
	dm2dSetLineTo(matrix, 0, t.sup);
	dm2dSetLineTo(matrix, N+1, t.inf);
	dm2dSetColumnTo(matrix, 0, t.esq);
	dm2dSetColumnTo(matrix, N+1, t.dir);

	dm2dCopy(matrix_aux, matrix);

	/* Lancemos as tarefas trabalhadoras */
	if (trab > 1) {
		if (inicializarMPlib(csz, trab) == -1) {
			fprintf(stderr, "Erro ao inicializar a MPlib.");
			exit(1);
		}
	}

	/* Lancemos a simulação e mostramos o resultado */
	result = simul(matrix, matrix_aux, N+2, N+2, iter);
	dm2dPrint(result);

	/* Limpemos as nossas estruturas */
	if (trab > 1) {
		libertarMPlib();
	}
	dm2dFree(matrix);
	dm2dFree(matrix_aux);

	return 0;
}
