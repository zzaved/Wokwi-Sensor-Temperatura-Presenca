#include "pico/stdlib.h"
#include "config.h"
#include "sensor.h"
#include "wifi.h"
#include "http_client.h"
#include "timer_handler.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// ─── Serialização do payload JSON ────────────────────────────────────────────
//
// Monta o JSON manualmente conforme o formato esperado pelo backend Go.
// Isso demonstra a serialização customizada exigida no Caminho 2 do enunciado.

static void montar_json_temperatura(char *buf, size_t max, float temp) {
    snprintf(buf, max,
        "{"
        "\"id_dispositivo\":%d,"
        "\"tipo_sensor\":\"temperatura\","
        "\"natureza_leitura\":\"analogica\","
        "\"valor_coletado\":\"%.2f\""
        "}",
        DEVICE_ID, temp
    );
}

static void montar_json_presenca(char *buf, size_t max, bool presente) {
    snprintf(buf, max,
        "{"
        "\"id_dispositivo\":%d,"
        "\"tipo_sensor\":\"presenca\","
        "\"natureza_leitura\":\"discreta\","
        "\"valor_coletado\":\"%s\""
        "}",
        DEVICE_ID, presente ? "1" : "0"
    );
}

// ─── Envio com retry ──────────────────────────────────────────────────────────

static bool enviar_com_retry(const char *json) {
    for (int tentativa = 1; tentativa <= HTTP_RETRY_MAX; tentativa++) {
        printf("[main] enviando (tentativa %d/%d): %s\n",
               tentativa, HTTP_RETRY_MAX, json);

        HttpResultado resultado = http_post(
            BACKEND_HOST, BACKEND_PORT, BACKEND_PATH, json
        );

        if (resultado == HTTP_OK) {
            printf("[main] envio OK\n");
            return true;
        }

        printf("[main] falha no envio (código %d), aguardando %dms...\n",
               resultado, HTTP_RETRY_DELAY_MS);

        sleep_ms(HTTP_RETRY_DELAY_MS);
    }

    printf("[main] ERRO: todas as tentativas falharam\n");
    return false;
}

// ─── Ponto de entrada ─────────────────────────────────────────────────────────

int main(void) {
    // Inicializa I/O serial (USB) para logs — aparece no terminal do Wokwi
    stdio_init_all();
    sleep_ms(2000); // aguarda USB enumerar no Wokwi

    printf("\n========================================\n");
    printf("  Pico W — Telemetria Industrial\n");
    printf("  Ponderada S07 — Caminho 2 (Pico SDK)\n");
    printf("========================================\n\n");

    // Inicializa sensores (GPIO + ADC + IRQ)
    sensor_init();

    // Inicializa timer de hardware para leitura periódica
    timer_init();

    // Conecta ao Wi-Fi
    if (!wifi_conectar()) {
        printf("[main] FATAL: não foi possível conectar ao Wi-Fi\n");
        // Pisca o LED onboard para indicar erro
        while (true) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            sleep_ms(200);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            sleep_ms(200);
        }
    }

    // LED aceso = Wi-Fi conectado
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    printf("\n[main] sistema pronto — enviando telemetria a cada %d ms\n\n",
           INTERVALO_LEITURA_MS);

    bool ultima_presenca = false;

    // ─── Loop principal ────────────────────────────────────────────────────────
    //
    // O loop não bloqueia: verifica flags setadas por IRQ e timer,
    // e chama cyw43_arch_poll() para processar o stack de rede.
    // Esse é o modelo de execução cooperativo do lwIP no Pico W.

    while (true) {
        // Mantém o stack TCP/IP processando eventos de rede
        cyw43_arch_poll();

        // Garante que o Wi-Fi ainda está conectado
        if (!wifi_garantir_conexao()) {
            printf("[main] sem Wi-Fi, aguardando reconexão...\n");
            sleep_ms(1000);
            continue;
        }

        // Lê os sensores
        LeituraSensores leitura = sensor_ler();

        char json[256];

        // ── Envio por evento: presença mudou de estado ─────────────────────
        // Independente do timer — enviamos imediatamente quando há mudança
        if (leitura.presenca_mudou) {
            printf("[main] mudança de presença detectada: %s\n",
                   leitura.presenca ? "DETECTADA" : "AUSENTE");

            montar_json_presenca(json, sizeof(json), leitura.presenca);
            enviar_com_retry(json);
            ultima_presenca = leitura.presenca;
        }

        // ── Envio periódico: timer disparou ────────────────────────────────
        if (timer_disparou()) {
            printf("[main] --- leitura periódica ---\n");
            printf("[main] temperatura: %.2f°C\n", leitura.temperatura_c);
            printf("[main] presença: %s\n", leitura.presenca ? "sim" : "não");

            // Envia temperatura
            montar_json_temperatura(json, sizeof(json), leitura.temperatura_c);
            enviar_com_retry(json);

            // Envia presença periodicamente também (estado atual)
            montar_json_presenca(json, sizeof(json), leitura.presenca);
            enviar_com_retry(json);
        }

        // Pequena pausa para não saturar o processador
        sleep_ms(10);
    }

    return 0; // nunca alcançado
}
