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
typedef struct barrier_t {
	pthread_mutex_t cond_mutex;
	pthread_cond_t wait_for_iteration;
	size_t size, count;
} barrier_t;

/* Variáveis globais */
barrier_t barrier;                   /* A nossa barreira */
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

#define str_or_null(a) a ? a : "null"

/* Helpers for (un)locking mutexes */
void lock_or_exit(pthread_mutex_t *mutex) {
	if (pthread_mutex_lock(mutex)) {
		fprintf(stderr, "\nErro ao bloquear mutex\n");
		exit(EXIT_FAILURE);
	}
}

void unlock_or_exit(pthread_mutex_t *mutex) {
	if (pthread_mutex_unlock(mutex)) {
		fprintf(stderr, "\nErro ao desbloquear mutex\n");
		exit(EXIT_FAILURE);
	}
}

/*--------------------------------------------------------------------
| Functions: Initializing and cleaning multithreading material
| - barrier_init(barrier, size)
|   | Initializes given barrier with given size
|
| - barrier_deinit(barrier)
|   | Destroys given barrier
|
| - barrier_wait(barrier, tid)
|   | Waits with given barrier.
|   | returns true (1) if all threads were unlocked, false (0) otherwise
---------------------------------------------------------------------*/
int barrier_init(barrier_t *barrier, size_t size) {
	if (pthread_mutex_init(&barrier->cond_mutex, NULL)) {
		fprintf(stderr, "\nErro ao inicializar wait_mutex\n");
		return 1;
	}

	if (pthread_cond_init(&barrier->wait_for_iteration, NULL)) {
		fprintf(stderr, "\nErro ao inicializar variável de condição\n");
		return 2;
	}

	barrier->size = size;
	barrier->count = 0;
	return 0;
}

int barrier_deinit(barrier_t *barrier) {
	if (pthread_mutex_destroy(&barrier->cond_mutex)) {
		fprintf(stderr, "\nErro ao inicializar wait_mutex\n");
		return 1;
	}

	if (pthread_cond_destroy(&barrier->wait_for_iteration)) {
		fprintf(stderr, "\nErro ao destruir variável de condição\n");
		return 2;
	}

	return 0;
}

int barrier_wait(barrier_t *barrier) {
	int retval = 0;

	barrier->count++;

	/* Se o contador for inferior ao tamanho, bloquear thread */
	if (barrier->count < barrier->size) {
		if (pthread_cond_wait(&barrier->wait_for_iteration, &barrier->cond_mutex)) {
			fprintf(stderr, "\nErro ao esperar pela variável de condição\n");
			exit(EXIT_FAILURE);
		}
	}
	/* Caso contrário, desbloqueia todas as threads */
	else {
		barrier->count = 0;
		if (pthread_cond_broadcast(&barrier->wait_for_iteration)) {
			fprintf(stderr, "\nErro ao desbloquear variável de condição\n");
			exit(EXIT_FAILURE);
		}

		retval = 1;
	}

	return retval;
}

/*--------------------------------------------------------------------
| Function: simul
---------------------------------------------------------------------*/
DoubleMatrix2D *simul(
	int first,
	int linhas,
	int colunas,
	int it,
	double maxD
) {
	DoubleMatrix2D *result = matrix;
	DoubleMatrix2D *matrix_temp = NULL;

	while (it-- > 0) {
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

		/* Monta a barreira para verificar se o maxD foi atingido */
		lock_or_exit(&barrier.cond_mutex);
		barrier_wait(&barrier);

		/* Verificamos se maxD foi atingido */
		if (dm2dDelimited(matrix, matrix_aux, colunas, maxD)) {
			unlock_or_exit(&barrier.cond_mutex);
			break;
		}

		/* Monta a barreira para poder trocar os ponteiros das matrizes */
		if (barrier_wait(&barrier)) {
			matrix_temp = matrix;
			matrix = matrix_aux;
			matrix_aux = matrix_temp;
		}
		unlock_or_exit(&barrier.cond_mutex);
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
	int n_l = simul_arg->first + simul_arg->k + 2;
	int n_c = simul_arg->N + 2;

	/* Fazer cálculos */
	DoubleMatrix2D *result = simul(
		simul_arg->first,
		n_l,
		n_c,
		simul_arg->iter,
		simul_arg->maxD
	);
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
| Function: File handling
| - file_exists()
| - open_or_create_new_matrix_file()
---------------------------------------------------------------------*/

/* Material de manuseamento de ficheiros */
#define F_EXISTS 1
#define F_NOT_EXISTS 0
#define F_ERROR -1

int file_exists(FILE *f) {
	if (f != NULL) {
		return F_EXISTS;
	}
	if (errno == ENOENT) {
		return F_NOT_EXISTS;
	}
	return F_ERROR;
}

FILE *open_or_create_new_matrix_file(const char* filename, int *read_mode) {
	FILE *f = fopen(filename, "r");

	/* Check if file exists */
	int state = file_exists(f);
	if (state == F_EXISTS) {
		/* close the stream and reopen it to read+write */
		fclose(f);
		f = fopen(filename, "rw");
		/* if opening THIS stream caused an error, exit the program */
		if (f == NULL) {
			fprintf(stderr, "Não foi possível escrever sobre \"%s\"\n", filename);
			return NULL;
		}

		*read_mode = 1;
	} else if (state == F_ERROR) {
		fprintf(stderr, "Não foi possível abrir \"%s\"\n", filename);
		return NULL;
	} else {
		/* Opening stream for write-only */
		f = fopen(filename, "w");
		/* if opening THIS stream caused an error, exit the program */
		if (f == NULL) {
			fprintf(stderr, "Não foi possível escrever sobre \"%s\"\n", filename);
			return NULL;
		}
	}

	return f;
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
	if (argc != 7 && argc != 9 && argc != 10 && argc != 12) {
		fprintf(stderr, "\nNúmero de Argumentos Inválido.\n");
		fprintf(stderr,
			"Utilização: heatSim N tEsq tSup tDir tInf iter trab csz maxD fichS periodoS\n\n"
		);
		return 1;
	}

	/* argv[0] = program name */
	/* Parsing integers */
	int N        = parse_integer_or_exit(argv[1], "N");
	int iter     = parse_integer_or_exit(argv[6], "iter");
	int trab     = 1;
	int csz      = 0;
	int periodoS = 0;
	if (8 <= argc) {
		trab     = parse_integer_or_exit(argv[7], "trab");
		csz      = parse_integer_or_exit(argv[8], "csz");
	}
	if (11 <= argc) {
		periodoS = parse_integer_or_exit(argv[11], "periodoS");
	}

	/* Parsing doubles */
	double maxD = 0;
	struct { double esq, sup, dir, inf; } t;
	t.esq = parse_double_or_exit(argv[2], "tEsq");
	t.sup = parse_double_or_exit(argv[3], "tSup");
	t.dir = parse_double_or_exit(argv[4], "tDir");
	t.inf = parse_double_or_exit(argv[5], "tInf");
	if (10 <= argc) {
		maxD = parse_double_or_exit(argv[9], "maxD");
	}

	/* Parsing other arguments */
	int read_mode = 0;
	const char* fichS = NULL;
	FILE *matrixFile = NULL;
	if (11 <= argc) {
		/* Opening the file appropriately */
		matrixFile = open_or_create_new_matrix_file(argv[10], &read_mode);
		if (matrixFile != NULL) {
			fichS = argv[10];
		}
	}

	fprintf(stderr, "\nArgumentos:\n"
		" N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d trab=%d csz=%d maxD=%.3f"
		" fichS=%s periodoS=%d",
		  N, t.esq, t.sup, t.dir, t.inf, iter, trab, csz, maxD, str_or_null(fichS), periodoS
	);

	/* VERIFICAR SE ARGUMENTOS ESTÃO CONFORME O ENUNCIADO */
	struct {
		double arg, val;
		const char* name;
	} arg_checker[] = {
		{ N,        1, "N"        },
		{ t.esq,    0, "tEsq"     },
		{ t.sup,    0, "tSup"     },
		{ t.dir,    0, "tDir"     },
		{ t.inf,    0, "tInf"     },
		{ iter,     1, "iter"     },
		{ trab,     1, "trab"     },
		{ csz,      0, "csz"      },
		{ maxD,     0, "maxD"     },
		{ periodoS, 0, "periodoS" }
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

	/* Inicializamos o nosso material mutlithreading */
	if (barrier_init(&barrier, trab)) {
		return EXIT_FAILURE;
	}

	/* Criar matrizes */
	matrix_aux = dm2dNew(N+2, N+2);
	if (is_arg_null(matrix_aux, "Erro ao criar Matrix2d.")) {
		return -1;
	}

	/* Preenchendo a nossa matriz de acordo com os argumentos */
	if (!read_mode) {
		matrix = dm2dNew(N+2, N+2);
		if (is_arg_null(matrix, "Erro ao criar Matrix2d.")) {
			return EXIT_FAILURE;
		}

		dm2dSetLineTo(matrix, 0, t.sup);
		dm2dSetLineTo(matrix, N+1, t.inf);
		dm2dSetColumnTo(matrix, 0, t.esq);
		dm2dSetColumnTo(matrix, N+1, t.dir);
	} else {
		matrix = readMatrix2dFromFile(matrixFile, N+2, N+2);
	}

	dm2dCopy(matrix_aux, matrix);
	DoubleMatrix2D *result = matrix;

	/* Preparemos as trabalhadoras */
	pthread_t *slaves = malloc(trab * sizeof(*slaves));
	if (is_arg_null(slaves, "Erro ao alocar memória para escravos.")) {
		return EXIT_FAILURE;
	}

	simul_args_t *slave_args = malloc(trab * sizeof(*slave_args));
	if (is_arg_null(slave_args, "Erro ao alocar memória para escravos.")) {
		return EXIT_FAILURE;
	}

	/* Começamos por inicializar as tarefas uma a uma */
	int idx = 0;
	for (int linha = 0; idx < trab; idx++, linha += k) {
		slave_args[idx].id = idx+1;
		slave_args[idx].first = linha;
		slave_args[idx].N = N;
		slave_args[idx].k = k;
		slave_args[idx].iter = iter;
		slave_args[idx].maxD = maxD;

		/* Verificando se a trabalhadora foi correctamente criada */
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
	if (barrier_deinit(&barrier)) {
		return EXIT_FAILURE;
	}
	free(slaves);
	free(slave_args);
	dm2dFree(matrix);
	dm2dFree(matrix_aux);

	fclose(matrixFile);

	return EXIT_SUCCESS;
}
