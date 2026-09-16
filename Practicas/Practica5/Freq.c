#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

// Definición de pines
#define SIG_PIN 15    // Entrada: señal a medir
#define TRAZA_PIN 14  // Salida: traza de la ISR para medir en osciloscopio

// Variables globales compartidas entre la ISR y el bucle principal
static volatile uint32_t t_subida_prev = 0;
static volatile uint32_t t_bajada = 0;
static volatile uint32_t T_ticks = 0;
static volatile uint32_t alto_ticks = 0;
static volatile bool listo = false;

// Rutina de interrupción ( ISR)
static void sig_isr(uint gpio, uint32_t events) {
    uint32_t t = timer_hw->timerawl;          // 1) Captura inmediata de la marca de tiempo[cite: 1]
    sio_hw->gpio_set = 1u << TRAZA_PIN;       // 2) Sube la señal de traza[cite: 1]

    if (events & GPIO_IRQ_EDGE_RISE) {
        if (t_subida_prev != 0) {             // Evita procesar basura en el primer flanco[cite: 1]
            T_ticks = t - t_subida_prev;
            alto_ticks = t_bajada - t_subida_prev;
            listo = true;
        }
        t_subida_prev = t;
    } else if (events & GPIO_IRQ_EDGE_FALL) {
        t_bajada = t;
    }
   //busy_wait_ms(50);

    sio_hw->gpio_clr = 1u << TRAZA_PIN;       // Baja la señal de traza[cite: 1]
    gpio_acknowledge_irq(gpio, events);
}

int main(void) {
    stdio_init_all();

    // Configuración del pin de traza
    gpio_init(TRAZA_PIN);
    gpio_set_dir(TRAZA_PIN, GPIO_OUT);
    gpio_put(TRAZA_PIN, 0);                   // Inicia en bajo[cite: 1]

    // Configuración del pin de señal
    gpio_init(SIG_PIN);
    gpio_set_dir(SIG_PIN, GPIO_IN);
    gpio_disable_pulls(SIG_PIN);              // Se deshabilita porque el nivel lo impone la fuente externa[cite: 1]
    
    // Habilitar interrupción para ambos flancos en un mismo pin[cite: 1]
    gpio_set_irq_enabled_with_callback(SIG_PIN, 
                                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                       true, &sig_isr);

    // Bucle principal
    while (true) {
        if (listo) {
            // Snapshot local para evitar que la ISR reescriba a mitad del cálculo[cite: 1]
            uint32_t T = T_ticks;
            uint32_t alto = alto_ticks;
            listo = false; 

            // Cálculos en punto flotante fuera de la ISR
            float f_hz = 1e6f / (float)T;     // El tick base es de 1 µs[cite: 1]
            float duty = 100.0f * (float)alto / (float)T; 

            printf("T = %u us | f = %.2f Hz | duty = %.1f %%\n", T, f_hz, duty);
            sleep_ms(200);                    // Límite de salida a consola para no saturarla[cite: 1]
        }
    }
    
    return 0;
}