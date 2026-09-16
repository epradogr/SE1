#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/structs/sio.h"
#define BOTON 14
#define LED 25
#define VENTANA_MS 20

typedef enum {
    ESTABLE_ALTO, POSIBLE_BAJO, ESTABLE_BAJO, POSIBLE_ALTO
} estado_t;

int main(void) {
   
    gpio_init(BOTON);
    gpio_pull_up(BOTON);
    
    gpio_init(LED);

    // oe_clr apaga la salida, volviendo al pin una entrada
    sio_hw->gpio_oe_clr = (1ul << BOTON);
    
    // oe_setactiva la salida
    sio_hw->gpio_oe_set = (1ul << LED);

    estado_t estado = ESTABLE_ALTO;
    absolute_time_t t_cambio;

    while (true) {
       
        // una mascara AND para aislar unicamente el bit de nuestro botón.
        bool nivel_alto = (sio_hw->gpio_in & (1ul << BOTON)) != 0;

        switch (estado) {
            case ESTABLE_ALTO:
                if (!nivel_alto) {
                    estado = POSIBLE_BAJO;
                    t_cambio = get_absolute_time();
                }
                break;

            case POSIBLE_BAJO:
                if (nivel_alto) {
                    estado = ESTABLE_ALTO;
                } else if (absolute_time_diff_us(t_cambio, get_absolute_time()) > VENTANA_MS * 1000) {
                    estado = ESTABLE_BAJO;
                    
                    
                    //Usamos el registro togl, que invierte el estado a nivel de hardware
                    sio_hw->gpio_togl = (1ul << LED);
                }
                break;

            case ESTABLE_BAJO:
                if (nivel_alto) {
                    estado = POSIBLE_ALTO;
                    t_cambio = get_absolute_time();
                }
                break;

            case POSIBLE_ALTO:
                if (!nivel_alto) {
                    estado = ESTABLE_BAJO;
                } else if (absolute_time_diff_us(t_cambio, get_absolute_time()) > VENTANA_MS * 1000) {
                    estado = ESTABLE_ALTO;
                    
                    
                    sio_hw->gpio_togl = (1ul << LED);
                }
                break;
        }
    }
}