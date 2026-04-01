#include "http_client.h"
#include "config.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include <stdio.h>
#include <string.h>

// ─── Estado interno da conexão TCP ───────────────────────────────────────────

typedef struct {
    struct tcp_pcb *pcb;        // bloco de controle do protocolo TCP
    bool           concluido;   // sinaliza fim da operação
    bool           erro;        // sinaliza erro
    bool           respondeu;   // servidor enviou resposta
    char           resposta[64]; // primeiros bytes da resposta HTTP
} EstadoTCP;

// ─── Callbacks do lwIP ────────────────────────────────────────────────────────
//
// O lwIP usa callbacks para notificar eventos de rede de forma assíncrona.
// Cada callback recebe o argumento passado em tcp_arg() — nosso EstadoTCP.

static err_t _cb_recebido(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    EstadoTCP *estado = (EstadoTCP *)arg;

    if (p == NULL) {
        // Conexão fechada pelo servidor
        estado->concluido = true;
        tcp_close(pcb);
        return ERR_OK;
    }

    if (err == ERR_OK) {
        // Copia os primeiros bytes da resposta para verificar status HTTP
        uint16_t copiar = p->len < 63 ? p->len : 63;
        memcpy(estado->resposta, p->payload, copiar);
        estado->resposta[copiar] = '\0';
        estado->respondeu = true;

        // Verifica se a resposta é 2xx (202 Accepted esperado)
        if (strstr(estado->resposta, "HTTP/1.1 2") == NULL &&
            strstr(estado->resposta, "HTTP/1.0 2") == NULL) {
            printf("[http] resposta inesperada: %.40s\n", estado->resposta);
            estado->erro = true;
        }

        tcp_recved(pcb, p->tot_len); // informa lwIP que processamos os dados
    }

    pbuf_free(p);
    estado->concluido = true;
    return ERR_OK;
}

static err_t _cb_erro_conn(void *arg, err_t err) {
    EstadoTCP *estado = (EstadoTCP *)arg;
    printf("[http] erro de conexão TCP: %d\n", err);
    estado->erro     = true;
    estado->concluido = true;
    return ERR_OK;
}

static void _cb_erro(void *arg, err_t err) {
    EstadoTCP *estado = (EstadoTCP *)arg;
    printf("[http] erro TCP: %d\n", err);
    estado->erro      = true;
    estado->concluido = true;
}

static err_t _cb_conectado(void *arg, struct tcp_pcb *pcb, err_t err) {
    EstadoTCP *estado = (EstadoTCP *)arg;

    if (err != ERR_OK) {
        printf("[http] falha ao conectar: %d\n", err);
        estado->erro      = true;
        estado->concluido = true;
        return err;
    }

    // Monta a requisição HTTP/1.1 manualmente
    // Isso demonstra a serialização customizada exigida no Caminho 2
    extern const char *_http_body_global;
    extern const char *_http_path_global;

    char request[512];
    int body_len = strlen(_http_body_global);

    snprintf(request, sizeof(request),
        "POST %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        _http_path_global,
        BACKEND_HOST, BACKEND_PORT,
        body_len,
        _http_body_global
    );

    // Escreve a requisição no buffer TCP e envia
    err_t write_err = tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    if (write_err != ERR_OK) {
        printf("[http] erro ao escrever no buffer TCP: %d\n", write_err);
        estado->erro      = true;
        estado->concluido = true;
        return write_err;
    }

    tcp_output(pcb); // força o envio imediato
    return ERR_OK;
}

// Variáveis globais para passar o body/path para o callback conectado
// (limitação do modelo de callbacks do lwIP sem closure)
const char *_http_body_global = NULL;
const char *_http_path_global = NULL;

// ─── Função principal de POST ─────────────────────────────────────────────────

HttpResultado http_post(const char *host, int porta, const char *path,
                        const char *json_body) {
    // Salva globais para uso no callback
    _http_body_global = json_body;
    _http_path_global = path;

    // Resolve DNS
    ip_addr_t servidor_ip;
    err_t dns_err = dns_gethostbyname(host, &servidor_ip, NULL, NULL);

    if (dns_err == ERR_INPROGRESS) {
        // DNS assíncrono — aguarda com polling do lwIP
        uint32_t inicio = to_ms_since_boot(get_absolute_time());
        while (dns_err == ERR_INPROGRESS) {
            cyw43_arch_poll();
            sleep_ms(10);
            if (to_ms_since_boot(get_absolute_time()) - inicio > 5000) {
                printf("[http] timeout na resolução DNS\n");
                return HTTP_ERRO_TIMEOUT;
            }
        }
    }

    if (dns_err != ERR_OK) {
        printf("[http] erro DNS: %d\n", dns_err);
        return HTTP_ERRO_CONEXAO;
    }

    // Cria o PCB (Protocol Control Block) TCP
    EstadoTCP estado = {0};
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb) {
        printf("[http] erro ao criar PCB TCP\n");
        return HTTP_ERRO_CONEXAO;
    }

    // Registra os callbacks e o argumento de contexto
    estado.pcb = pcb;
    tcp_arg(pcb, &estado);
    tcp_recv(pcb, _cb_recebido);
    tcp_err(pcb, _cb_erro);

    // Inicia conexão TCP
    err_t conn_err = tcp_connect(pcb, &servidor_ip, porta, _cb_conectado);
    if (conn_err != ERR_OK) {
        printf("[http] erro ao iniciar conexão: %d\n", conn_err);
        tcp_abort(pcb);
        return HTTP_ERRO_CONEXAO;
    }

    // Aguarda conclusão com polling do lwIP (modelo não-bloqueante)
    uint32_t inicio = to_ms_since_boot(get_absolute_time());
    while (!estado.concluido) {
        cyw43_arch_poll();
        sleep_ms(5);
        if (to_ms_since_boot(get_absolute_time()) - inicio > 10000) {
            printf("[http] timeout aguardando resposta\n");
            tcp_abort(pcb);
            return HTTP_ERRO_TIMEOUT;
        }
    }

    return (estado.erro || !estado.respondeu) ? HTTP_ERRO_RESPOSTA : HTTP_OK;
}
