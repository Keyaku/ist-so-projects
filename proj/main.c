/*
// Projeto SO - exercicio 1, version 03
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "files.h"
#include "barrier.h"
#include "signals.h"
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

/* Variáveis globais */
barrier_t barrier;                   /* A nossa barreira */
DoubleMatrix2D *matrix, *matrix_aux; /* As nossas duas matrizes */

const char *fichS; /* Nome do ficheiro de salvaguarda */
char *fichS_temp;  /* Nome do ficheiro de salvaguarda temporário */
int periodoS;      /* Período de salvaguarda */

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

/* Helpers for child process */
void wait_properly() {
	int wstatus = 0;
	waitpid(-1, &wstatus, WNOHANG);
	if (!WIFEXITED(wstatus)) {
		fprintf(stderr, "Processo filho de salvaguarda não terminou correctamente\n");
	}
	// FIXME: add more wstatus checks?
}

/* Slightly safer approach to writing matrix to a file: writes to temporary first. */
void safe_write_matrix() {
	/* Saving to temporary file */
	writeMatrix2dToFile(fichS_temp, matrix);

	/* Renaming to proper filename */
	if (rename(fichS_temp, fichS)) {
		fprintf(stderr, "Não foi possível rescrever ficheiro temporário de \"%s\"\n", fichS_temp);
	}
}

void clean_globals() {
	free(fichS_temp);

	/* Libertar a barreira */
	if (barrier_deinit(&barrier)) {
		exit(EXIT_FAILURE);
	}

	/* Libertar matrizes */
	dm2dFree(matrix);
	dm2dFree(matrix_aux);
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

	while (it-- > 0 && !signals_was_interrupted()) {
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
			/* Verificar a próxima salvaguarda */
			if (signals_was_alarmed()) {
				/* Esperar pelo filho anterior antes de continuar */
				wait_properly();

				/* Lançar novo processo salvaguarda */
				pid_t save_child = fork();
				if (save_child == 0) {
					safe_write_matrix();
					exit(EXIT_SUCCESS);
				} else if (save_child == -1) {
					fprintf(stderr,
						"Não foi possível criar processo filho.\n"
						"Não será salva-guardado esta vez.\n"
					);
				}

				/* Reiniciar contador */
				signals_reset_alarm(periodoS);
			}

			/* Efectuar troca de ponteiros */
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
	periodoS     = 0;
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
	int state = 0;
	fichS = NULL;
	fichS_temp = NULL;
	if (11 <= argc) {
		/* Opening the file appropriately */
		state = file_exists(argv[10]);
		if (state == F_ERROR) {
			fprintf(stderr, "Não foi possível verificar a existência de \"%s\"", argv[10]);
		} else {
			fichS = argv[10];
			size_t len = strlen(fichS);

			fichS_temp = malloc(len + 2);
			if (fichS_temp == NULL) {
				fprintf(stderr, "Erro ao alocar memória para \"%s\"\n", fichS);
				return EXIT_FAILURE;
			}

			/* Composing temporary filename */
			strcpy(fichS_temp, fichS);
			fichS_temp[len] = '~'; fichS_temp[len+1] = 0;
		}
	}

	fprintf(stderr, "\nArgumentos:\n"
		" N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d trab=%d csz=%d maxD=%.3f"
		" fichS=%s periodoS=%d"
		"\n",
		  N,   t.esq,    t.sup,    t.dir,    t.inf,    iter,   trab,   csz,   maxD,
		str_or_null(fichS), periodoS
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

	/* Inicializamos os sinais a usar */
	signals_init(periodoS);

	/* Inicializamos o nosso material multithreading */
	if (barrier_init(&barrier, trab)) {
		return EXIT_FAILURE;
	}

	/* Criar matrizes */
	matrix_aux = dm2dNew(N+2, N+2);
	if (is_arg_null(matrix_aux, "Erro ao criar Matrix2d.")) {
		return EXIT_FAILURE;
	}

	/* Preenchendo a nossa matriz de acordo com o ficheiro */
	if (fichS != NULL && state == F_EXISTS) {
		FILE *matrix_file = fopen(fichS, "r");
		if (matrix_file == NULL) {
			fprintf(stderr, "Não foi possível abrir \"%s\"\n", fichS);
		} else {
			matrix = readMatrix2dFromFile(matrix_file, N+2, N+2);
		}
	}

	/* Preenchendo a nossa matriz de acordo com os argumentos */
	if (matrix == NULL) {
		matrix = dm2dNew(N+2, N+2);
		if (is_arg_null(matrix, "Erro ao criar Matrix2d.")) {
			return EXIT_FAILURE;
		}

		dm2dSetLineTo(matrix, 0, t.sup);
		dm2dSetLineTo(matrix, N+1, t.inf);
		dm2dSetColumnTo(matrix, 0, t.esq);
		dm2dSetColumnTo(matrix, N+1, t.dir);
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

	/* Esperar por qualquer processo-filho uma última vez */
	wait_properly();

	/* Mostramos o resultado */
	if (is_arg_null(result, "result")) {
		return EXIT_FAILURE;
	}
	dm2dPrint(result);

	/* Salvaguardar em caso de Interrupção; apagar caso contrário */
	if (signals_was_interrupted()) {
		safe_write_matrix();
	} else {
		file_delete(fichS);
	}

	/* Limpar estruturas */
	clean_globals();
	free(slaves);
	free(slave_args);

	return EXIT_SUCCESS;
}
