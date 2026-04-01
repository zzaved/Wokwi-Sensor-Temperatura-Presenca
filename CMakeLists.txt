cmake_minimum_required(VERSION 3.13)

# Importa o SDK do Pico — precisa estar antes do project()
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico_telemetria C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pico_telemetria
    src/main.c
    src/wifi.c
    src/http_client.c
    src/sensor.c
    src/timer_handler.c
)

# Inclui os headers do projeto
target_include_directories(pico_telemetria PRIVATE src/)

# Bibliotecas do Pico SDK que usamos:
# - pico_stdlib         → funções base (gpio, uart, tempo)
# - pico_cyw43_arch     → driver Wi-Fi do chip CYW43 (presente no Pico W)
# - hardware_adc        → periférica ADC para sensor analógico
# - hardware_timer      → timer de hardware para leitura periódica
# - pico_lwip_http      → stack TCP/IP lwIP com suporte HTTP
target_link_libraries(pico_telemetria
    pico_stdlib
    pico_cyw43_arch_lwip_poll
    hardware_adc
    hardware_timer
    pico_lwip_http
)

# Habilita saída serial via USB (para logs/debug no Wokwi)
pico_enable_stdio_usb(pico_telemetria 1)
pico_enable_stdio_uart(pico_telemetria 0)

# Gera os arquivos .uf2 e .bin para gravação/simulação
pico_add_extra_outputs(pico_telemetria)
