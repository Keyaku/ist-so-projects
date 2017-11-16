#ifndef SIGNALS_H
#define SIGNALS_H


void signals_init();
int signals_was_interrupted();
int signals_was_alarmed();
void signals_reset_alarm();

#endif // SIGNALS_H
