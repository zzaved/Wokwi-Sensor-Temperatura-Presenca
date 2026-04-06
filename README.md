# Ponderada S07 — Atividade 2: Firmware Pico W

Firmware embarcado para Raspberry Pi Pico W que lê dois sensores e envia telemetria para o backend desenvolvido na [Atividade 1](https://github.com/zzaved/iot-sensor-go).

## Framework e toolchain

**Caminho 2 — Toolchain Nativo (Pico SDK)**

Utiliza o Pico SDK oficial com C puro, compilação via CMake e `arm-none-eabi-gcc`. Nenhum framework de abstração (Arduino, MicroPython) foi utilizado — toda a comunicação com periféricos é feita diretamente pelas APIs do SDK.

## Sensores integrados

| Sensor | Tipo | GPIO | Natureza | Range |
|---|---|---|---|---|
| NTC Temperature Sensor (TMP36) | Analógico | GP26 (ADC canal 0) | `analogica` | -40°C a 125°C |
| Botão / Sensor de presença | Digital | GP15 | `discreta` | `0` ou `1` |

**Sensor de temperatura (GP26):**
O ADC do Pico W é de 12 bits (0–4095). A conversão segue a fórmula do TMP36:
```
tensão = raw * (3.3 / 4096)
temperatura_C = (tensão - 0.5) / 0.01
```
A leitura usa média móvel de 8 amostras para suavizar ruído analógico.

Sensor de presença (GP15):
Configurado com pull-down interno e IRQ nas bordas de subida e descida.
Implementa debounce por software de 50ms para filtrar ruído elétrico.
Envia telemetria imediatamente quando detecta mudança de estado (event-driven), além do envio periódico pelo timer.

## Diagrama de conexão

```
Raspberry Pi Pico W
┌──────────────────────────┐
│                          │
│  3V3 ────────────────────┼──── VCC (TMP36)
│  GND ────────────────────┼──── GND (TMP36)
│  GP26 (ADC0) ────────────┼──── OUT (TMP36)
│                          │
│  3V3 ──── R(10kΩ) ───────┼──── GP15 ──── Botão ──── GND
│                          │
└──────────────────────────┘
```

O resistor de 10kΩ forma um divisor de tensão com o botão (pull-up externo). O pull-down interno do Pico também é habilitado via software como redundância.

## Estrutura do projeto

```
pico-telemetria/
├── src/
│   ├── main.c              # loop principal + serialização JSON
│   ├── sensor.c/h          # ADC + GPIO + IRQ + média móvel
│   ├── wifi.c/h            # máquina de estados Wi-Fi (CYW43)
│   ├── http_client.c/h     # cliente HTTP customizado (lwIP raw API)
│   ├── timer_handler.c/h   # timer de hardware para leitura periódica
│   └── config.h            # todas as configurações em um lugar
├── CMakeLists.txt
├── diagram.json            # diagrama Wokwi
├── wokwi.toml              # configuração do simulador
└── README.md
```

## Compilação

### Pré-requisitos

```bash
# macOS
brew install cmake arm-none-eabi-gcc

# Pico SDK (necessário)
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk && git submodule update --init
export PICO_SDK_PATH=/caminho/para/pico-sdk
```

O arquivo `build/pico_telemetria.uf2` é gerado para gravação no hardware físico.
O arquivo `build/pico_telemetria.elf` é usado pelo Wokwi para simulação.


## Arquitetura Cloud

O backend da Atividade 1 foi implantado na nuvem usando serviços gratuitos:

```
Pico W / curl
     │ HTTP POST /telemetria
     ▼
[Relay local :8080]          ← go run ./cmd/relay/main.go
     │ HTTPS
     ▼
API — Cloud Run              ← telemetria-api-338668699937.us-central1.run.app
     │ amqps://
     ▼
RabbitMQ — CloudAMQP         ← Little Lemur (free tier)
     │
     ▼
Worker — Cloud Run           ← consome fila e persiste
     │ postgresql://
     ▼
PostgreSQL — Neon            ← neondb (free tier)
```

### Serviços utilizados (todos free tier)

| Serviço | Plataforma | Observação |
|---|---|---|
| API Go | Google Cloud Run | 2M req/mês grátis |
| Worker Go | Google Cloud Run | consumer da fila |
| Message broker | CloudAMQP — Little Lemur | 1M mensagens/mês grátis |
| Banco de dados | Neon PostgreSQL | 0.5 GB grátis |

### Por que o relay local?

O firmware do Pico W usa TCP puro (lwIP raw API) sem suporte a TLS. O Cloud Run aceita apenas HTTPS. O relay resolve isso: aceita HTTP na porta 8080 e encaminha via HTTPS para o Cloud Run internamente.

O simulador Wokwi requer licença paga para o network bridge (que conecta `host.wokwi.internal` ao localhost). Em hardware físico real, o Pico W se conectaria diretamente ao relay.

### Executando com relay

```bash
# Terminal 1 — relay HTTP → Cloud Run
cd iot-sensor-go
go run ./cmd/relay/main.go

# Terminal 2 — monitorar banco em tempo real
watch -n 3 psql "postgresql://..." -c "SELECT tipo_sensor, valor_coletado, criado_em FROM telemetria_sensores ORDER BY criado_em DESC LIMIT 5;"
```

### Testando o pipeline completo via curl

```bash
# Envia leitura de temperatura
curl -s -X POST https://telemetria-api-338668699937.us-central1.run.app/telemetria \
  -H "Content-Type: application/json" \
  -d '{"id_dispositivo":42,"tipo_sensor":"temperatura","natureza_leitura":"analogica","valor_coletado":"26.50"}'

# Envia leitura de presença
curl -s -X POST https://telemetria-api-338668699937.us-central1.run.app/telemetria \
  -H "Content-Type: application/json" \
  -d '{"id_dispositivo":42,"tipo_sensor":"presenca","natureza_leitura":"discreta","valor_coletado":"1"}'
```

## Simulação no Wokwi

1. Instale a extensão **Wokwi** no VS Code
2. Compile o projeto (gera o `.elf`)
3. Abra `diagram.json` no VS Code
4. Pressione `F1` → **Wokwi: Start Simulator**

> **Importante:** O Wokwi conecta ao backend local via `host.wokwi.internal`. Certifique-se que o backend da Atividade 1 está rodando com `docker compose up` antes de iniciar a simulação.

**Interagindo com os sensores no Wokwi:**
- **Temperatura:** clique no sensor NTC e arraste o slider para mudar a temperatura simulada
- **Presença:** clique no botão verde para simular detecção de presença

## Configuração de rede

Todas as configurações ficam em `src/config.h`:

```c
#define WIFI_SSID       "WokwiFi"        // SSID da rede
#define WIFI_PASSWORD   "wokwi2024"      // senha

#define BACKEND_HOST    "host.wokwi.internal"  // localhost da sua máquina
#define BACKEND_PORT    8080
#define BACKEND_PATH    "/telemetria"

#define DEVICE_ID       42               // ID único deste dispositivo
#define INTERVALO_LEITURA_MS  5000       // envia a cada 5 segundos
```

## Formato da telemetria enviada

O firmware envia dois tipos de payload, exatamente no formato esperado pelo backend da Atividade 1:

**Temperatura:**
```json
{
  "id_dispositivo": 42,
  "tipo_sensor": "temperatura",
  "natureza_leitura": "analogica",
  "valor_coletado": "25.30"
}
```

**Presença:**
```json
{
  "id_dispositivo": 42,
  "tipo_sensor": "presenca",
  "natureza_leitura": "discreta",
  "valor_coletado": "1"
}
```

## Funcionalidades implementadas (critérios de avaliação)

**Sensor digital com IRQ e debounce (2pts)**
- GPIO configurado com pull-down interno
- IRQ nas bordas de subida e descida (`GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL`)
- Debounce por software de 50ms no handler da interrupção
- Envio event-driven imediato ao detectar mudança

**Sensor analógico com ADC (2pts)**
- ADC de 12 bits configurado diretamente via `hardware_adc`
- Conversão de valor bruto para temperatura em °C (fórmula TMP36)
- Média móvel de 8 amostras para suavização de leitura

**Wi-Fi com máquina de estados (2pts)**
- Integração direta com driver CYW43 e stack lwIP
- Máquina de estados com detecção de link loss e reconexão automática
- Timeout de 30s na tentativa de conexão

**Cliente HTTP customizado + retry (2pts)**
- Implementação sobre lwIP raw TCP API (sem biblioteca HTTP de terceiros)
- Serialização manual de JSON
- Mecanismo de retry com 3 tentativas e backoff de 1s

**Timer de hardware (2pts extras — Caminho 2)**
- `repeating_timer` do SDK com intervalo negativo para precisão exata
- Leitura periódica sem bloquear o loop principal
- Loop principal cooperativo com `cyw43_arch_poll()`
