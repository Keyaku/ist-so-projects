/*
** Logger
** Sistemas Operativos, Antonio Sarmento, 77906, 2016-2017
*/

#ifndef BANK_LOGGER_H
#define BANK_LOGGER_H

/* Public methods */
void init_logger(void);
void destroy_logger(void);
void write_to_log(unsigned long thread, const char *str);

#endif /* BANK_LOGGER_H */
