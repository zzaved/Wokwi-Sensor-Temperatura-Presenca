#ifndef SENSOR_H
#define SENSOR_H

#include <stdbool.h>
#include <stdint.h>

// Leitura consolidada dos dois sensores
typedef struct {
    float   temperatura_c;   // graus Celsius
    bool    presenca;        // true = presença detectada
    bool    presenca_mudou;  // true se houve mudança desde a última leitura
} LeituraSensores;

// Inicializa GPIO do sensor de presença e ADC do sensor de temperatura
void sensor_init(void);

// Lê os dois sensores e retorna os valores atuais
LeituraSensores sensor_ler(void);

// Retorna o estado atual do sensor de presença (chamado pela IRQ)
bool sensor_presenca_estado(void);

#endif // SENSOR_H
