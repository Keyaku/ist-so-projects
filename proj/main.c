/*
// Projeto SO - exercicio 1, version 03
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "matrix2d.h"


/* Protótipos */
int is_arg_null(void *arg, const char *name);

/* Material de tarefas */
typedef struct simul_args_t {
	int id, first;
	int N;       /* Tamanho de linhas/colunas */
	int k;       /* Número de linhas a processar por tarefa trabalhadora */
	int iter;    /* Número de iterações a processar */
	double maxD; /* Limiar de travagem */

} simul_args_t;
void *slave_thread(void *arg);

/* Material de concorrência */
// TODO

/* Variáveis globais */
DoubleMatrix2D *matrix, *matrix_aux; /* As nossas duas matrizes */

/*--------------------------------------------------------------------
| Helper functions
---------------------------------------------------------------------*/
double average(double *array, size_t size) {
	double result = 0;

	for (size_t idx = 0; idx < size; idx++) {
		result += array[idx];
	}

	return result / size;
}

#define array_len(arr) sizeof arr / sizeof *arr
#define max(a, b) a < b ? b : a

/*--------------------------------------------------------------------
| Function: simul
---------------------------------------------------------------------*/
DoubleMatrix2D *simul(
	int first,
	int linhas,
	int colunas,
	int it,
	int id,
	double maxD
) {
	DoubleMatrix2D *result = matrix;
	DoubleMatrix2D *matrix_temp = NULL;

	/* Diferencial para o lumiar de travagem */
	double d = 0;
	int is_done = 0;

	while (it-- > 0 && !is_done) {
		/* Processamos uma iteração */
		for (int i = first+1; i < linhas-1; i++) {
			for (int j = 1; j < colunas-1; j++) {
				double arr[] = {
					dm2dGetEntry(matrix, i-1, j),
					dm2dGetEntry(matrix, i,   j-1),
					dm2dGetEntry(matrix, i+1, j),
					dm2dGetEntry(matrix, i,   j+1)
				};
				double value = average(arr, array_len(arr));
				dm2dSetEntry(matrix_aux, i, j, value);
			}
		}

		/* Verificamos se maxD foi atingido */
		if (maxD != 0) {
			d = 0;
			for (int j = 1; j < colunas-1; j++) {
				double diff = dm2dGetEntry(matrix_aux, first+1, j) - dm2dGetEntry(matrix, first+1, j);
				d = max(d, diff);
			}
			is_done = d < maxD;
		}

		/* Switching pointers between matrices, avoids boilerplate code */
		matrix_temp = matrix;
		matrix = matrix_aux;
		matrix_aux = matrix_temp;
	}

	/* Se houve uma confusão de pointers, resolvemos o de matrix_aux */
	if (result != matrix) {
		matrix_aux = matrix;
		matrix = result;
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
| Uma mini-matriz de (k+2) x (N+2). Graças a esta estratégia que usa partilha
| de memória, não perdemos tempo nem espaço a alocar cada fatia.
---------------------------------------------------------------------*/
void *slave_thread(void *arg) {
	simul_args_t *simul_arg = (simul_args_t*) arg;
	int id      = simul_arg->id;
	int first   = simul_arg->first;
	int N       = simul_arg->N;
	int k       = simul_arg->k;
	int iter    = simul_arg->iter;
	double maxD = simul_arg->maxD;

	/* Fazer cálculos */
	DoubleMatrix2D *result = simul(first, k+2, N+2, iter, id, maxD);
	if (is_arg_null(result, "result (thread)")) {
		return NULL;
	}

	return result;
}

/*--------------------------------------------------------------------
| Function: Parsing from standard input
| - parse_integer_or_exit()
| - parse_double_or_exit()
---------------------------------------------------------------------*/
int parse_integer_or_exit(const char *str, const char *name) {
	int value;

	if (sscanf(str, "%d", &value) != 1) {
		fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
		exit(1);
	}
	return value;
}

double parse_double_or_exit(const char *str, const char *name) {
	double value;

	if (sscanf(str, "%lf", &value) != 1) {
		fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
		exit(1);
	}
	return value;
}

/*--------------------------------------------------------------------
| Function: Arguments checking
| - is_arg_greater_equal_to(value, greater, name)
| - is_arg_null(argument, name)
---------------------------------------------------------------------*/
void is_arg_greater_equal_to(int value, int greater, const char *name) {
	if (value < greater) {
		fprintf(stderr, "\n%s tem que ser >= %d\n\n", name, greater);
		exit(1);
	}
}

int is_arg_null(void *arg, const char *msg) {
	if (arg == NULL) {
		fprintf(stderr, "\n%s\n", msg);
		return 1;
	}
	return 0;
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
int main(int argc, char *argv[]) {
	if (argc != 7 && argc != 9 && argc != 10) {
		fprintf(stderr, "\nNúmero de Argumentos Inválido.\n");
		fprintf(stderr, "Utilização: heatSim N tEsq tSup tDir tInf iter trab csz maxD\n\n");
		return 1;
	}

	// Auxiliary variables
	int retval = 0;

	/* argv[0] = program name */
	int N    = parse_integer_or_exit(argv[1], "N");
	int iter = parse_integer_or_exit(argv[6], "iter");
	int trab = 1;
	int csz  = 0;
	if (8 <= argc && argc <= 9) {
		trab = parse_integer_or_exit(argv[7], "trab");
		csz  = parse_integer_or_exit(argv[8], "csz");
	}

	double maxD = 0;
	struct { double esq, sup, dir, inf; } t;
	t.esq = parse_double_or_exit(argv[2], "tEsq");
	t.sup = parse_double_or_exit(argv[3], "tSup");
	t.dir = parse_double_or_exit(argv[4], "tDir");
	t.inf = parse_double_or_exit(argv[5], "tInf");
	if (10 <= argc) {
		maxD = parse_double_or_exit(argv[7], "maxD");
	}

	fprintf(stderr, "\nArgumentos:\n"
		" N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d trab=%d csz=%d maxD=%.1f",
		N, t.esq, t.sup, t.dir, t.inf, iter, trab, csz, maxD
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
		{ maxD,  0, "maxD" },
		{ trab,  1, "trab" },
		{ csz,   0, "csz"  }
	};
	for (size_t idx = 0; idx < array_len(arg_checker); idx++) {
		is_arg_greater_equal_to(arg_checker[idx].arg, arg_checker[idx].val, arg_checker[idx].name);
	}

	int k = N/trab;
	if ((double) k != (double) N/trab) {
		fprintf(stderr, "\nErro: Argumento %s e %s invalidos\n"
			"%s deve ser multiplo de %s.", "N", "trab", "N", "trab"
		);
		return -1;
	}

	/* Criar matrizes */
	matrix = dm2dNew(N+2, N+2);
	if (is_arg_null(matrix, "Erro ao criar Matrix2d.")) {
		return -1;
	}
	matrix_aux = dm2dNew(N+2, N+2);
	if (is_arg_null(matrix_aux, "Erro ao criar Matrix2d.")) {
		dm2dFree(matrix);
		return -1;
	}

	/* Preenchendo a nossa matriz de acordo com os argumentos */
	dm2dSetLineTo(matrix, 0, t.sup);
	dm2dSetLineTo(matrix, N+1, t.inf);
	dm2dSetColumnTo(matrix, 0, t.esq);
	dm2dSetColumnTo(matrix, N+1, t.dir);
	dm2dCopy(matrix_aux, matrix);

	/* Lancemos a simulação */
	DoubleMatrix2D *result = matrix;
	size_t idx;
	int linha = 0;

	/* Inicializar tarefas uma a uma */
	pthread_t *slaves = malloc(trab * sizeof(*slaves));
	if (is_arg_null(slaves, "Erro ao alocar memória para escravos.")) {
		return -1;
	}

	simul_args_t *slave_args = malloc(trab * sizeof(*slave_args));
	if (is_arg_null(slave_args, "Erro ao alocar memória para escravos.")) {
		return -1;
	}

	/* Primeiro set de iterações */
	for (idx = 0; idx < trab; idx++, linha += k) {
		slave_args[idx].id = idx+1;
		slave_args[idx].first = linha;
		slave_args[idx].N = N;
		slave_args[idx].k = k;
		slave_args[idx].iter = iter;

		/* Verificando se o fio de execução foi correctamente criado */
		if (pthread_create(&slaves[idx], NULL, slave_thread, &slave_args[idx])) {
			fprintf(stderr, "\nErro ao criar um escravo\n");
			return EXIT_FAILURE;
		}
	}

	/* "Juntar" tarefas trabalhadoras */
	for (idx = 0; idx < trab; idx++) {
		if (pthread_join(slaves[idx], NULL)) {
			fprintf(stderr, "\nErro ao esperar por um escravo.\n");
			return EXIT_FAILURE;
		}
	}

	if (is_arg_null(result, "result")) {
		return EXIT_FAILURE;
	}

	/* Mostramos o resultado */
	dm2dPrint(result);

	/* Limpar estruturas */
	free(slaves);
	free(slave_args);
	dm2dFree(matrix);
	dm2dFree(matrix_aux);

	return retval;
}
