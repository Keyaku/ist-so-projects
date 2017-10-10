/*
// Projeto SO - exercicio 1, version 03
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "matrix2d.h"
#include "mplib3.h"

#define ARRAY_LEN(arr) sizeof arr / sizeof *arr

/* Variáveis globais */
int N; /* Tamanho de linhas/colunas */
int k; /* Número de linhas a processar por tarefa trabalhadora */

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
void process_entry(
	DoubleMatrix2D *matrix,
	DoubleMatrix2D *matrix_aux,
	int i,
	int j
) {
	double arr[] = {
		dm2dGetEntry(matrix, i-1, j),
		dm2dGetEntry(matrix, i,   j-1),
		dm2dGetEntry(matrix, i+1, j),
		dm2dGetEntry(matrix, i,   j+1)
	};
	double value = average(arr, ARRAY_LEN(arr));
	dm2dSetEntry(matrix_aux, i, j, value);
}

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
				process_entry(matrix, matrix_aux, i, j);
			}
		}

		/* Switching pointers between matrices, avoids boilerplate code */
		matrix_temp = matrix;
		matrix = matrix_aux;
		matrix_aux = matrix_temp;
	}

	return result;
}

DoubleMatrix2D *simul_multithread(
	DoubleMatrix2D *matrix,
	DoubleMatrix2D *matrix_aux,
	int iter
) {
	DoubleMatrix2D *result = matrix;
	double *receive_slice = NULL;

	// TODO: Percorrer um dado número de iterações

	/* Enviar matriz-raíz linha-a-linha */
	for (int i = 0; i < N; i+=k) {
		for (size_t slave_id = 0; slave_id < k; slave_id++) {
			int l = i + slave_id;
			enviarMensagem(0, slave_id+1, dm2dGetLine(matrix, l), N);
		}
	}

	/* Receber matriz inteira calculada linha-a-linha */
	for (int i = 0; i < N; i+=k) {
		for (size_t slave_id = 0; slave_id < k; slave_id++) {
			int l = i + slave_id;
			receberMensagem(slave_id+1, 0, receive_slice, N);
			dm2dSetLine(result, l, receive_slice);
		}
	}

	return result;
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
| O que é uma fatia?
| Uma mini-matriz de (k+2) x (N+2). A maneira indicada, mas consome muito mais
| memória.
---------------------------------------------------------------------*/
typedef struct {
	int id;
} simul_args_t;

void *slave_thread(void *arg) {
	simul_args_t *bucket = (simul_args_t*) arg;
	double *receive_slice = NULL;
	int myid = bucket->id;

	/* Criar a nossa mini-matriz */
	DoubleMatrix2D *mini_matrix = dm2dNew(k+2, N+2);
	DoubleMatrix2D *mini_aux = NULL, *result = NULL;

	/* Receber a matriz linha a linha */
	for (int i = 0; i < k; i++) {
		receberMensagem(0, myid, receive_slice, N);
		dm2dSetLine(mini_matrix, i, receive_slice);
	}
	dm2dCopy(mini_aux, mini_matrix);

	/* Fazer cálculos */
	result = simul(mini_matrix, mini_aux, k+2, N+2, 1);

	/* Enviar matrix calculada linha a linha */
	for (int i = 0; i < k; i++) {
		enviarMensagem(myid, 0, dm2dGetLine(result, i), N);
	}

	dm2dFree(mini_matrix);
	dm2dFree(mini_aux);
	return 0;
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
| 4. [√] Esperar pela terminação das threads
| 5. [√] Imprimir resultado e libertar memória (usar valgrind)
|
---------------------------------------------------------------------*/
int main (int argc, char *argv[]) {
	if (argc != 7 && argc != 9) {
		fprintf(stderr, "\nNumero invalido de argumentos.\n");
		fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iter trab csz\n\n");
		return 1;
	}

	// Auxiliary variables
	size_t idx;

	/* argv[0] = program name */
	N        = parse_integer_or_exit(argv[1], "N");
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
	for (idx = 0; idx < ARRAY_LEN(arg_checker); idx++) {
		is_arg_greater_equal_to(arg_checker[idx].arg, arg_checker[idx].val, arg_checker[idx].name);
	}

	k = N/trab;
	if ((double) k != (double) N/trab) {
		fprintf(stderr, "\ntrab não é um múltiplo de N\n");
		return -1;
	}

	/* Criar matrizes */
	DoubleMatrix2D *matrix, *matrix_aux;
	matrix = dm2dNew(N+2, N+2);
	matrix_aux = dm2dNew(N+2, N+2);

	/* FAZER ALTERACOES AQUI */
	dm2dSetLineTo(matrix, 0, t.sup);
	dm2dSetLineTo(matrix, N+1, t.inf);
	dm2dSetColumnTo(matrix, 0, t.esq);
	dm2dSetColumnTo(matrix, N+1, t.dir);

	dm2dCopy(matrix_aux, matrix);

	/* Lancemos as tarefas trabalhadoras */
	pthread_t *slaves = NULL;
	simul_args_t *slave_args = NULL;
	if (trab > 1) {
		slaves     = malloc(trab * sizeof(*slaves));
		slave_args = malloc(trab * sizeof(*slave_args));

		if (inicializarMPlib(csz, trab) == -1) {
			fprintf(stderr, "Erro ao inicializar a MPlib.");
			return -1;
		}

		for (idx = 0; idx < trab; idx++) {
			slave_args[idx].id = idx+1;

			pthread_create(&slaves[idx], NULL, slave_thread, &slave_args[idx]);
		}
	}

	/* Lancemos a simulação e mostramos o resultado */
	DoubleMatrix2D *result;
	if (trab > 1) {
		result = simul_multithread(matrix, matrix_aux, iter);
	} else {
		result = simul(matrix, matrix_aux, N+2, N+2, iter);
	}
	dm2dPrint(result);

	/* Limpemos as nossas estruturas */
	if (trab > 1) {
		for (idx = 0; idx < trab; idx++) {
			if (pthread_join(slaves[idx], NULL)) {
				fprintf(stderr, "\nErro ao esperar por um escravo.\n");
				return -1;
			}
		}

		libertarMPlib();
	}
	free(slaves);
	free(slave_args);
	dm2dFree(matrix);
	dm2dFree(matrix_aux);

	return 0;
}
