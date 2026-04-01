#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stdbool.h>

// Resultado do envio
typedef enum {
    HTTP_OK,
    HTTP_ERRO_CONEXAO,
    HTTP_ERRO_TIMEOUT,
    HTTP_ERRO_RESPOSTA
} HttpResultado;

// Envia um POST com corpo JSON para o backend
// Retorna HTTP_OK se o servidor respondeu 2xx
HttpResultado http_post(const char *host, int porta, const char *path,
                        const char *json_body);

#endif // HTTP_CLIENT_H
