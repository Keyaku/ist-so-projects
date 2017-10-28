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

/* Protótipos */
int is_arg_null(void *arg, const char *name);

/* Estruturas pessoais */
void *slave_thread(void *arg);

/* Variáveis globais */
static size_t buffer_size;
static int N;    /* Tamanho de linhas/colunas */
static int k;    /* Número de linhas a processar por tarefa trabalhadora */
static int iter; /* Número de iterações a processar */
static int trab; /* Número de trabalhadoras a lançar */
static double maxD; /* Limiar de travagem */

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

#define max(a, b) a < b ? b : a

/*--------------------------------------------------------------------
| Function: simul
---------------------------------------------------------------------*/
DoubleMatrix2D *simul(
	DoubleMatrix2D *matrix,
	int linhas,
	int colunas,
	int it,
	int id
) {
	DoubleMatrix2D *result = matrix;
	DoubleMatrix2D *matrix_temp = NULL;
	double *receive_slice = malloc(buffer_size);
	int is_slave = linhas != colunas;

	/* Diferencial para o lumiar de travagem */
	double d = 0;
	int is_done = 0;

	/* Criando uma matriz auxiliar */
	DoubleMatrix2D *matrix_aux = dm2dNew(linhas, colunas);

	/* Verificar se as alocações foram bem sucedidas */
	if (is_arg_null(receive_slice, "receive_slice")) {
		return NULL;
	}
	if (is_arg_null(matrix_aux, "matrix_aux")) {
		free(receive_slice);
		return NULL;
	}

	dm2dCopy(matrix_aux, matrix);

	while (it-- > 0 && !is_done) {
		/* Processamos uma iteração */
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

		/* Verificamos se maxD foi atingido */
		d = 0;
		for (int j = 1; j < colunas-1; j++) {
			double diff = dm2dGetEntry(matrix_aux, 1, j) - dm2dGetEntry(matrix, 1, j);
			d = max(d, diff);
		}
		is_done = d < maxD;

		/* Switching pointers between matrices, avoids boilerplate code */
		matrix_temp = matrix;
		matrix = matrix_aux;
		matrix_aux = matrix_temp;

		/* Enviamos a primeira linha e última a trabalhadoras vizinhas */
		if (is_slave) {
			if (id > 1) {
				receberMensagem(id-1, id, receive_slice, buffer_size);
				dm2dSetLine(matrix, 0, receive_slice);
				enviarMensagem(id, id-1, dm2dGetLine(matrix, 1), buffer_size);
			}
			if (id < trab) {
				enviarMensagem(id, id+1, dm2dGetLine(matrix, linhas-2), buffer_size);
				receberMensagem(id+1, id, receive_slice, buffer_size);
				dm2dSetLine(matrix, linhas-1, receive_slice);
			}
		}
	}

	/* Se houve uma confusão de pointers, resolvemos o de matrix_aux */
	if (result != matrix) {
		matrix_aux = matrix;
	}

	free(receive_slice);
	dm2dFree(matrix_aux);
	return result;
}

DoubleMatrix2D *simul_multithread(DoubleMatrix2D *matrix) {
	DoubleMatrix2D *result = matrix;
	double *receive_slice = malloc(buffer_size);
	size_t idx;
	int linha = 0;

	/* Inicializar tarefas uma a uma */
	pthread_t *slaves     = malloc(trab * sizeof(*slaves));
	int       *slave_args = malloc(trab * sizeof(*slave_args));

	/* Verificar se as alocações foram bem sucedidas */
	if (is_arg_null(receive_slice, "receive_slice")) {
		return NULL;
	}
	if (is_arg_null(slaves, "slaves")) {
		free(receive_slice);
		return NULL;
	}
	if (is_arg_null(slave_args, "slave_args")) {
		free(receive_slice);
		free(slaves);
		return NULL;
	}

	/* Primeiro set de iterações */
	for (idx = 0; idx < trab; idx++) {
		slave_args[idx] = idx+1;

		/* Verificando se o fio de execução foi correctamente criado */
		if (pthread_create(&slaves[idx], NULL, slave_thread, &slave_args[idx])) {
			fprintf(stderr, "\nErro ao criar um escravo\n");

			free(receive_slice);
			free(slaves);
			free(slave_args);

			return NULL;
		}

		/* Enviar matriz-raíz linha-a-linha */
		for (int i = 0; i < k+2; i++) {
			enviarMensagem(0, idx+1, dm2dGetLine(matrix, linha++), buffer_size);
		}
		linha -= 2;
	}

	/* Receber matriz inteira calculada linha-a-linha */
	linha = 1;
	for (idx = 0; idx < trab; idx++) {
		for (int i = 0; i < k; i++) {
			receberMensagem(idx+1, 0, receive_slice, buffer_size);
			dm2dSetLine(result, linha++, receive_slice);
		}
	}

	/* "Juntar" tarefas trabalhadoras */
	for (idx = 0; idx < trab; idx++) {
		if (pthread_join(slaves[idx], NULL)) {
			fprintf(stderr, "\nErro ao esperar por um escravo.\n");

			result = NULL;
			break;
		}
	}

	/* Limpar estruturas */
	free(receive_slice);
	free(slaves);
	free(slave_args);

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
void *slave_thread(void *arg) {
	double *receive_slice = malloc(buffer_size);
	int id = *(int*) arg;

	/* Criar a nossa mini-matriz */
	DoubleMatrix2D *mini_matrix = dm2dNew(k+2, N+2);
	DoubleMatrix2D *result = NULL;

	if (is_arg_null(receive_slice, "receive_slice (thread)")) {
		return NULL;
	}
	if (is_arg_null(mini_matrix, "mini_matrix (thread)")) {
		free(receive_slice);
		return NULL;
	}

	/* Receber a matriz linha a linha */
	for (int i = 0; i < k+2; i++) {
		receberMensagem(0, id, receive_slice, buffer_size);
		dm2dSetLine(mini_matrix, i, receive_slice);
	}

	/* Fazer cálculos */
	result = simul(mini_matrix, k+2, N+2, iter, id);
	if (is_arg_null(result, "result (thread)")) {
		free(receive_slice);
		dm2dFree(mini_matrix);
		return NULL;
	}

	/* Enviar matrix calculada linha a linha */
	for (int i = 1; i < k+1; i++) {
		enviarMensagem(id, 0, dm2dGetLine(result, i), buffer_size);
	}

	free(receive_slice);
	dm2dFree(mini_matrix);
	return 0;
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

int is_arg_null(void *arg, const char *name) {
	if (arg == NULL) {
		fprintf(stderr, "\nErro ao alocar memória para \"%s\"\n\n", name);
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
	if (argc != 8 && argc != 10) {
		fprintf(stderr, "\nNumero invalido de argumentos.\n");
		fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iter maxD (trab csz)\n\n");
		return 1;
	}

	// Auxiliary variables
	size_t idx;

	/* argv[0] = program name */
	N       = parse_integer_or_exit(argv[1], "N");
	iter    = parse_integer_or_exit(argv[6], "iter");
	trab    = 1;
	int csz = 0;
	if (9 <= argc && argc <= 10) {
		trab = parse_integer_or_exit(argv[8], "trab");
		csz  = parse_integer_or_exit(argv[9], "csz");
	}

	struct { double esq, sup, dir, inf; } t;
	t.esq = parse_double_or_exit(argv[2], "tEsq");
	t.sup = parse_double_or_exit(argv[3], "tSup");
	t.dir = parse_double_or_exit(argv[4], "tDir");
	t.inf = parse_double_or_exit(argv[5], "tInf");
	maxD  = parse_double_or_exit(argv[7], "maxD");

	fprintf(stderr, "\nArgumentos:\n"
		"    N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d maxD=%1.f trab=%d csz=%d\n",
		N, t.esq, t.sup, t.dir, t.inf, iter, maxD, trab, csz
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
		{ maxD,  1, "maxD" },
		{ trab,  1, "trab" },
		{ csz,   0, "csz"  }
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
	DoubleMatrix2D *matrix = dm2dNew(N+2, N+2);
	if (is_arg_null(matrix, "matrix")) {
		return -1;
	}

	/* Preenchendo a nossa matriz de acordo com os argumentos */
	dm2dSetLineTo(matrix, 0, t.sup);
	dm2dSetLineTo(matrix, N+1, t.inf);
	dm2dSetColumnTo(matrix, 0, t.esq);
	dm2dSetColumnTo(matrix, N+1, t.dir);

	/* Lancemos a simulação */
	DoubleMatrix2D *result;
	if (trab > 1) {
		if (inicializarMPlib(csz, trab+1) == -1) {
			fprintf(stderr, "\nErro ao inicializar a MPlib.\n");
			return -1;
		}

		buffer_size = (N+2) * sizeof(double); /* O nosso buffer tem que ser o número de colunas +2 */
		result = simul_multithread(matrix);

		libertarMPlib();
	} else {
		result = simul(matrix, N+2, N+2, iter, 0);
	}

	if (is_arg_null(result, "result")) {
		return -1;
	}

	/* Mostramos o resultado */
	dm2dPrint(result);

	dm2dFree(matrix);
	return 0;
}
