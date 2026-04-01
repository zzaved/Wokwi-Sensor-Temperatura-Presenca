#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>

// Estados da máquina de estados Wi-Fi
typedef enum {
    WIFI_DESCONECTADO,
    WIFI_CONECTANDO,
    WIFI_CONECTADO,
    WIFI_ERRO
} WiFiEstado;

// Inicializa o chip CYW43 e tenta conectar
bool wifi_conectar(void);

// Retorna o estado atual da conexão
WiFiEstado wifi_estado(void);

// Verifica conexão e reconecta se necessário (chamar periodicamente)
bool wifi_garantir_conexao(void);

#endif // WIFI_H
