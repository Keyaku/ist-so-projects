#ifndef SIGNALS_H
#define SIGNALS_H


void signals_init(int interval);
int signals_was_interrupted(void);
int signals_was_alarmed(void);
void signals_reset_alarm(int interval);

#endif // SIGNALS_H
