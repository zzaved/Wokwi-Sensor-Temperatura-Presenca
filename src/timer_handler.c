#include "timer_handler.h"
#include "config.h"
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include <stdio.h>

// Flag volatile — modificada dentro do callback do timer (contexto de IRQ)
static volatile bool _disparou = false;

// O repeating_timer mantém o estado do timer registrado
static struct repeating_timer _timer;

// ─── Callback do timer de hardware ───────────────────────────────────────────
//
// Chamado automaticamente pelo hardware a cada INTERVALO_LEITURA_MS.
// Apenas seta o flag — o processamento acontece no loop principal.
// Retorna true para manter o timer repetindo.

static bool _cb_timer(struct repeating_timer *t) {
    _disparou = true;
    return true;
}

// ─── Inicialização ────────────────────────────────────────────────────────────

void timer_init(void) {
    // Intervalo negativo = baseado no FIM da execução anterior
    // Garante que o intervalo seja exato entre disparos, sem drift
    add_repeating_timer_ms(
        -INTERVALO_LEITURA_MS,
        _cb_timer,
        NULL,
        &_timer
    );

    printf("[timer] timer de hardware configurado: %d ms\n", INTERVALO_LEITURA_MS);
}

// ─── Consulta e consumo do flag ───────────────────────────────────────────────

bool timer_disparou(void) {
    if (_disparou) {
        _disparou = false;
        return true;
    }
    return false;
}
