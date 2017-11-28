#ifndef HELPERS_H
#define HELPERS_H


// MACROS
#define array_len(arr) sizeof arr / sizeof *arr
#define max(a, b) a < b ? b : a

#define str_or_null(a) a ? a : "null"

// PROTOTYPES
double average(double *array, size_t size);
void wait_properly(pid_t pid, int flags);

#endif // HELPERS_H
