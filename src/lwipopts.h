#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Configuração lwIP para Pico W — HTTP client com DNS e TCP raw API

// Sistema sem OS (bare metal / FreeRTOS not used)
#define NO_SYS                      1
#define SYS_LIGHTWEIGHT_PROT        1

// DHCP e DNS necessários para conexão Wi-Fi e resolução de host
#define LWIP_DHCP                   1
#define LWIP_DNS                    1

// TCP/IP habilitados, UDP necessário para DHCP/DNS
#define LWIP_TCP                    1
#define LWIP_UDP                    1

// Desabilita APIs de alto nível (não usadas — usamos raw API diretamente)
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0

// Callback de status de interface de rede
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_HOSTNAME         1

// Habilita ICMP para diagnóstico
#define LWIP_ICMP                   1
#define LWIP_RAW                    1

// Tamanhos de buffer TCP/IP
#define MEM_SIZE                    4000
#define MEMP_NUM_TCP_SEG            16
#define MEMP_NUM_TCP_PCB            4
#define TCP_MSS                     1460
#define TCP_SND_BUF                 (4 * TCP_MSS)
#define TCP_SND_QUEUELEN            8
#define TCP_WND                     (4 * TCP_MSS)

// Configura pbuf
#define PBUF_POOL_SIZE              24

// Necessário para cyw43_arch_poll (modo poll sem thread)
#define LWIP_NETIF_LOOPBACK         0

#endif // _LWIPOPTS_H
