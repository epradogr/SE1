#include <stdio.h>
#include "pico/stdlib.h"

#define BUTTON_PIN 15
#define TRACE_PIN  16

// ISR del botón
void gpio_callback(uint gpio, uint32_t events) {
    gpio_put(TRACE_PIN, 1);   // marca inicio de la ISR (fin de latencia)

    busy_wait_us(200);        // trabajo simulado (tiempo de servicio)

    gpio_put(TRACE_PIN, 0);   // marca fin de la ISR
}

int main() {
    stdio_init_all();

    // Pin de traza (Canal 2)
    gpio_init(TRACE_PIN);
    gpio_set_dir(TRACE_PIN, GPIO_OUT);
    gpio_put(TRACE_PIN, 0);

    // Pin del botón (Canal 1)
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // Habilita la IRQ en flanco de bajada
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    while (true) {
        tight_loop_contents();
    }
    return 0;
}