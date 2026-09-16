#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define BTN_PIN     16
#define LED_PIN     25
#define VENTANA_MS  20
#define TEST_PIN    17 // Pin extra para medir el tiempo de ejecución de la ISR

// Se ejecuta VENTANA_MS después del primer flanco
static int64_t fin_debounce(alarm_id_t id, void *user_data) {
    if (gpio_get(BTN_PIN) == 0) {        // activo en bajo
        gpio_xor_mask(1u << LED_PIN);
    }
    gpio_set_irq_enabled(BTN_PIN, GPIO_IRQ_EDGE_FALL, true);
    return 0;                            // no reprogramar
}

// Rutina de Servicio de Interrupción (ISR)
static void btn_isr(uint gpio, uint32_t events) {
    // INICIO DE MEDICIÓN: Ponemos el pin de prueba en ALTO
    gpio_put(TEST_PIN, 1); 

    gpio_set_irq_enabled(BTN_PIN, GPIO_IRQ_EDGE_FALL, false);
    add_alarm_in_ms(VENTANA_MS, fin_debounce, NULL, true);
    gpio_acknowledge_irq(gpio, events);

    // FIN DE MEDICIÓN: Ponemos el pin de prueba en BAJO
    gpio_put(TEST_PIN, 0); 
}

int main() {
    stdio_init_all();

    // 1. Configuración del LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // 2. Configuración del Pin de Prueba (Para el analizador lógico)
    gpio_init(TEST_PIN);
    gpio_set_dir(TEST_PIN, GPIO_OUT);
    gpio_put(TEST_PIN, 0); // Inicia en bajo

    // 3. Configuración del Botón
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN); // Habilitar Pull-up interno para que lea '1' cuando no está presionado

    // 4. Configurar e habilitar la interrupción
    gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_FALL, true, &btn_isr);

    // 5. Bucle infinito
    while (true) {
        tight_loop_contents(); // Instrucción de bajo consumo mientras espera interrupciones
    }

    return 0;
}