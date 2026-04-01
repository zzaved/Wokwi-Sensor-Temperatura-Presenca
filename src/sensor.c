#include "sensor.h"
#include "config.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <stdio.h>

// ─── Estado interno ──────────────────────────────────────────────────────────

// Variável volatile porque é modificada dentro de uma IRQ (interrupção)
// Sem volatile, o compilador pode otimizá-la e nunca reler da memória
static volatile bool     _presenca_atual  = false;
static volatile bool     _presenca_mudou  = false;
static volatile uint64_t _ultimo_irq_us   = 0;  // timestamp do último IRQ (para debounce)

// Buffer da média móvel para suavizar leituras analógicas
static uint16_t _buffer_adc[MEDIA_MOVEL_N] = {0};
static uint8_t  _buffer_idx = 0;
static bool     _buffer_cheio = false;

// ─── IRQ handler do sensor de presença ───────────────────────────────────────
//
// Chamado automaticamente pelo hardware quando o pino muda de estado.
// ATENÇÃO: handlers de IRQ precisam ser rápidos — sem printf, sem sleep aqui.
// Apenas lemos o estado e atualizamos as variáveis compartilhadas.

static void _irq_presenca(uint gpio, uint32_t eventos) {
    uint64_t agora = time_us_64();

    // Debounce por software: ignora IRQs muito próximas no tempo
    // Isso filtra ruído elétrico (bouncing) do botão/sensor
    if ((agora - _ultimo_irq_us) < DEBOUNCE_US) {
        return;
    }
    _ultimo_irq_us = agora;

    // Lê o nível atual do pino e detecta mudança
    bool novo_estado = gpio_get(gpio);
    if (novo_estado != _presenca_atual) {
        _presenca_atual = novo_estado;
        _presenca_mudou = true;
    }
}

// ─── Inicialização ────────────────────────────────────────────────────────────

void sensor_init(void) {
    // --- Sensor de presença (digital) ---
    gpio_init(PIN_PRESENCA);
    gpio_set_dir(PIN_PRESENCA, GPIO_IN);

    // Resistor de pull-down interno: pino em LOW quando nada conectado
    // Quando o sensor/botão pressiona, vai para HIGH (presença = true)
    gpio_pull_down(PIN_PRESENCA);

    // Registra a IRQ para detectar BORDA DE SUBIDA e DESCIDA
    // Isso captura tanto "chegou" quanto "foi embora"
    gpio_set_irq_enabled_with_callback(
        PIN_PRESENCA,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true,
        &_irq_presenca
    );

    printf("[sensor] presença inicializado no GPIO %d\n", PIN_PRESENCA);

    // --- Sensor de temperatura (analógico via ADC) ---
    adc_init();
    adc_gpio_init(PIN_TEMPERATURA);   // configura GPIO26 como entrada ADC
    adc_select_input(ADC_CANAL_TEMP); // seleciona canal 0

    printf("[sensor] ADC inicializado no GPIO %d (canal %d)\n",
           PIN_TEMPERATURA, ADC_CANAL_TEMP);
}

// ─── Leitura do ADC com média móvel ─────────────────────────────────────────

static float _ler_temperatura(void) {
    // Lê valor bruto do ADC (12 bits: 0 a 4095)
    uint16_t raw = adc_read();

    // Adiciona no buffer circular de média móvel
    _buffer_adc[_buffer_idx] = raw;
    _buffer_idx = (_buffer_idx + 1) % MEDIA_MOVEL_N;
    if (_buffer_idx == 0) _buffer_cheio = true;

    // Calcula média dos N valores acumulados
    uint8_t n = _buffer_cheio ? MEDIA_MOVEL_N : _buffer_idx;
    uint32_t soma = 0;
    for (uint8_t i = 0; i < n; i++) soma += _buffer_adc[i];
    uint16_t media = soma / n;

    // Converte ADC bruto → tensão → temperatura
    // Fórmula para sensor TMP36: V = raw * (3.3 / 4096)
    // Temperatura: T(°C) = (V - 0.5) / 0.01
    float tensao = (float)media * (ADC_VREF / ADC_RESOLUCAO);
    float temperatura = (tensao - 0.5f) / 0.01f;

    return temperatura;
}

// ─── Leitura consolidada ──────────────────────────────────────────────────────

LeituraSensores sensor_ler(void) {
    LeituraSensores leitura;

    leitura.temperatura_c  = _ler_temperatura();
    leitura.presenca       = _presenca_atual;
    leitura.presenca_mudou = _presenca_mudou;

    // Reseta a flag de mudança após leitura
    _presenca_mudou = false;

    return leitura;
}

bool sensor_presenca_estado(void) {
    return _presenca_atual;
}
