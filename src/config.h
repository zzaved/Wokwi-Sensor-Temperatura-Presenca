#ifndef CONFIG_H
#define CONFIG_H

// ─── Rede Wi-Fi ───────────────────────────────────────────────────────────────
// No Wokwi, qualquer SSID/senha funciona — a rede é simulada
#define WIFI_SSID       "WokwiFi"
#define WIFI_PASSWORD   "wokwi2024"

// ─── Backend (Atividade 1) ────────────────────────────────────────────────────
// Endereço do backend Go rodando localmente
// No Wokwi, o host "host.wokwi.internal" aponta para o localhost da sua máquina
#define BACKEND_HOST    "host.wokwi.internal"
#define BACKEND_PORT    8080
#define BACKEND_PATH    "/telemetria"

// ─── Identificação do dispositivo ────────────────────────────────────────────
#define DEVICE_ID       42

// ─── Pinos GPIO ───────────────────────────────────────────────────────────────
// Sensor de presença (digital) — pino GPIO 15
// No Wokwi: conectado a um botão que simula presença/ausência
#define PIN_PRESENCA    15

// Sensor de temperatura (analógico) — ADC canal 0 = GPIO 26
// No Pico W, os pinos ADC são: GPIO26(ch0), GPIO27(ch1), GPIO28(ch2)
#define PIN_TEMPERATURA 26
#define ADC_CANAL_TEMP  0

// ─── Timer ────────────────────────────────────────────────────────────────────
// Intervalo de leitura/envio em milissegundos
// 5000ms = lê e envia a cada 5 segundos
#define INTERVALO_LEITURA_MS  5000

// ─── Debounce ─────────────────────────────────────────────────────────────────
// Tempo mínimo em microsegundos entre duas interrupções do sensor de presença
// Evita leituras falsas causadas por ruído elétrico no botão/sensor
#define DEBOUNCE_US     50000   // 50ms

// ─── ADC ──────────────────────────────────────────────────────────────────────
// O ADC do Pico é de 12 bits (0 a 4095)
// Tensão de referência: 3.3V
// Fórmula: temperatura_C = (tensao_V - 0.5) / 0.01  (para sensor TMP36)
#define ADC_RESOLUCAO   4096.0f
#define ADC_VREF        3.3f

// Média móvel: número de amostras para suavizar leitura analógica
#define MEDIA_MOVEL_N   8

// ─── Retry ────────────────────────────────────────────────────────────────────
#define HTTP_RETRY_MAX      3
#define HTTP_RETRY_DELAY_MS 1000

#endif // CONFIG_H
