#ifndef TIMER_HANDLER_H
#define TIMER_HANDLER_H

#include <stdbool.h>

// Inicializa o timer de hardware para disparar leituras periódicas
void timer_init(void);

// Retorna true se o timer disparou desde a última chamada (consome o flag)
bool timer_disparou(void);

#endif // TIMER_HANDLER_H
