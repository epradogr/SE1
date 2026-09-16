#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/structs/sio.h"
#include "hardware/structs/timer.h" //timer_hw

#define LED_PIN 15
#define TRAZA_PIN 14

#define ALARM_NUM 0
#define ALARM_IRQ TIMER_IRQ_0

#define INTERVALO_US 250000u //250ms

volatile uint32_t next_deadline;

//handler de la alarma
void on_alarm_irq(void)
{
    //bajar bandera de interrupcion
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);

    //toggle leds
    sio_hw->gpio_togl = 1u << LED_PIN;
    sio_hw->gpio_togl = 1u << TRAZA_PIN;

    //reprogramar siguiente disparo
    next_deadline += INTERVALO_US;
    timer_hw->alarm[ALARM_NUM] = next_deadline;
}

int main(void)
{
    //init gpios
    gpio_init(LED_PIN);
    sio_hw->gpio_oe_set = 1u << LED_PIN;
    sio_hw->gpio_clr = 1u << LED_PIN;

    gpio_init(TRAZA_PIN);
    sio_hw->gpio_oe_set = 1u << TRAZA_PIN;
    sio_hw->gpio_clr = 1u << TRAZA_PIN;

    //configurar interrupcion del timer
    irq_set_exclusive_handler(ALARM_IRQ, on_alarm_irq);
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    irq_set_enabled(ALARM_IRQ, true);

    //arrancar primera alarma
    uint32_t ahora = timer_hw->timerawl;
    next_deadline = ahora + INTERVALO_US;
    timer_hw->alarm[ALARM_NUM] = next_deadline;

    while (true)
    {
        tight_loop_contents();
    }
}