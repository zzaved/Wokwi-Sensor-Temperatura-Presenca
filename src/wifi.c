#include "wifi.h"
#include "config.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>

// Estado atual da conexão
static WiFiEstado _estado = WIFI_DESCONECTADO;

// ─── Inicialização e conexão ──────────────────────────────────────────────────

bool wifi_conectar(void) {
    printf("[wifi] inicializando chip CYW43...\n");

    // Inicializa o chip Wi-Fi do Pico W
    // cyw43_arch_init() configura o driver e o stack lwIP
    if (cyw43_arch_init()) {
        printf("[wifi] ERRO: falha ao inicializar CYW43\n");
        _estado = WIFI_ERRO;
        return false;
    }

    // Modo station: conecta em uma rede existente (ao invés de criar um AP)
    cyw43_arch_enable_sta_mode();

    printf("[wifi] conectando em '%s'...\n", WIFI_SSID);
    _estado = WIFI_CONECTANDO;

    // Tenta conectar com timeout de 30 segundos
    // O terceiro parâmetro é o tipo de autenticação (WPA2 por padrão)
    int resultado = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID,
        WIFI_PASSWORD,
        CYW43_AUTH_WPA2_AES_PSK,
        30000
    );

    if (resultado != 0) {
        printf("[wifi] ERRO: não foi possível conectar (código %d)\n", resultado);
        _estado = WIFI_ERRO;
        return false;
    }

    _estado = WIFI_CONECTADO;
    printf("[wifi] conectado! IP: %s\n",
           ip4addr_ntoa(netif_ip4_addr(netif_default)));

    return true;
}

// ─── Estado atual ─────────────────────────────────────────────────────────────

WiFiEstado wifi_estado(void) {
    return _estado;
}

// ─── Reconexão automática ─────────────────────────────────────────────────────
//
// Verifica se ainda está conectado e tenta reconectar se necessário.
// Deve ser chamada no loop principal para garantir robustez.

bool wifi_garantir_conexao(void) {
    // Verifica link layer (nível físico da conexão Wi-Fi)
    if (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP) {
        _estado = WIFI_CONECTADO;
        return true;
    }

    // Link caiu — tenta reconectar
    printf("[wifi] conexão perdida, reconectando...\n");
    _estado = WIFI_DESCONECTADO;

    return wifi_conectar();
}
