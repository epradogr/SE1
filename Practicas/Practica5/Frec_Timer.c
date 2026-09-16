#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

// Definición de pines
#define SIG_PIN 15    // Entrada: señal a medir
#define TRAZA_PIN 14  // Salida: traza de la ISR
#define SYS_CLK 150e6f // Frecuencia de reloj del RP2350 (150 MHz)

// Contenedores globales para microsegundos
static volatile uint32_t t_subida_prev_us = 0;
static volatile uint32_t t_bajada_us = 0;
static volatile uint32_t T_us_ticks = 0;
static volatile uint32_t alto_us_ticks = 0;

// Contenedores globales para ciclos (TIMER1)
static volatile uint32_t t_subida_prev_cy = 0;
static volatile uint32_t t_bajada_cy = 0;
static volatile uint32_t T_cy_ticks = 0;
static volatile uint32_t alto_cy_ticks = 0;

static volatile bool listo = false;

// Rutina de interrupción (ISR)
static void sig_isr(uint gpio, uint32_t events) {
    // 1) Captura inmediata en ambas escalas
    uint32_t t_us = timer_hw->timerawl;       // Modo µs
    uint32_t t_cy = timer1_hw->timerawl;      // Modo ciclos (TIMER1)

    // 2) Sube la señal de traza
    sio_hw->gpio_set = 1u << TRAZA_PIN;

    if (events & GPIO_IRQ_EDGE_RISE) {
        if (t_subida_prev_us != 0) { 
            // Cálculos en microsegundos
            T_us_ticks = t_us - t_subida_prev_us;
            alto_us_ticks = t_bajada_us - t_subida_prev_us;
            
            // Cálculos en ciclos
            T_cy_ticks = t_cy - t_subida_prev_cy;
            alto_cy_ticks = t_bajada_cy - t_subida_prev_cy;
            
            listo = true;
        }
        t_subida_prev_us = t_us;
        t_subida_prev_cy = t_cy;
    } else if (events & GPIO_IRQ_EDGE_FALL) {
        t_bajada_us = t_us;
        t_bajada_cy = t_cy;
    }

    // 3) Baja la señal de traza
    sio_hw->gpio_clr = 1u << TRAZA_PIN;
    gpio_acknowledge_irq(gpio, events);
}

int main(void) {
    stdio_init_all();

    // Configuración del pin de traza
    gpio_init(TRAZA_PIN);
    gpio_set_dir(TRAZA_PIN, GPIO_OUT);
    gpio_put(TRAZA_PIN, 0);

    // Configuración del pin de señal
    gpio_init(SIG_PIN);
    gpio_set_dir(SIG_PIN, GPIO_IN);
    gpio_disable_pulls(SIG_PIN);
    
    // Habilitar interrupción para ambos flancos
    gpio_set_irq_enabled_with_callback(SIG_PIN, 
                                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                       true, &sig_isr);

    // Bucle principal
    while (true) {
        if (listo) {
            // Snapshot local para proteger los cálculos
            uint32_t T_us = T_us_ticks;
            uint32_t alto_us = alto_us_ticks;
            uint32_t T_cy = T_cy_ticks;
            uint32_t alto_cy = alto_cy_ticks;
            listo = false; 

            // Matemáticas modo µs
            float f_hz_us = 1e6f / (float)T_us;
            float duty_us = 100.0f * (float)alto_us / (float)T_us; 

            // Matemáticas modo ciclos
            float f_hz_cy = SYS_CLK / (float)T_cy; 
            float duty_cy = 100.0f * (float)alto_cy / (float)T_cy; 

            // Impresión comparativa
            printf("[uS] T = %u us | f = %.2f Hz | duty = %.1f %%\n", T_us, f_hz_us, duty_us);
            printf("[CY] T = %u cy | f = %.2f Hz | duty = %.1f %%\n", T_cy, f_hz_cy, duty_cy);
            printf("-------------------------------------------------\n");
            
            sleep_ms(200);
        }
    }
    
    return 0;
}