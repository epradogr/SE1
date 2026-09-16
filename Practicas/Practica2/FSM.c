#include <stdio.h>
#include "pico/stdlib.h"

//Definicion del pin del boton, LED y tiempo de antirrebote
#define BOTON 14
#define LED 25
#define VENTANA_MS 20

//Definicion de los estados de la maquina para filtrar ruido mecanico
typedef enum {
    ESTABLE_ALTO, POSIBLE_BAJO, ESTABLE_BAJO, POSIBLE_ALTO
} estado_t;

int main(void) {
    //Configuracion del pin del boton como entrada
    gpio_init(BOTON);
    gpio_set_dir(BOTON, false);
    //Habilita la resistencia interna para asegurar un 1 logico por defecto
    gpio_pull_up(BOTON);

    //Configuracion del pin del LED como salida
    gpio_init(LED);
    gpio_set_dir(LED, true);

    //Se inicializa el sistema asumiendo que el boton no esta presionado
    estado_t estado = ESTABLE_ALTO;
    absolute_time_t t_cambio;

    //Bucle infinito del microcontrolador
    while (true) {
        //Se lee constantemente el estado actual del pin del boton
        bool nivel_alto = gpio_get(BOTON);

        //Maquina de estados para evaluar transiciones limpias
        switch (estado) {
            case ESTABLE_ALTO:
                //Si lee un 0 logico, podria indicar que se presiono el boton
                if (!nivel_alto) {
                    estado = POSIBLE_BAJO;
                    t_cambio = get_absolute_time();
                }
                break;

            case POSIBLE_BAJO:
                //Si el boton vuelve a 1 repentinamente, fue un rebote electrico
                if (nivel_alto) {
                    estado = ESTABLE_ALTO;
                //Si se mantiene en 0 por mas de 20ms, la pulsacion es legitima
                } else if (absolute_time_diff_us(t_cambio, get_absolute_time()) > VENTANA_MS * 1000) {
                    estado = ESTABLE_BAJO;
                    //Se invierte el estado actual del LED
                    gpio_xor_mask(1u << LED);
                }
                break;

            case ESTABLE_BAJO:
                //Si lee un 1 logico, podria indicar que el usuario solto el boton
                if (nivel_alto) {
                    estado = POSIBLE_ALTO;
                    t_cambio = get_absolute_time();
                }
                break;

            case POSIBLE_ALTO:
                //Si el pin vuelve a 0, fue ruido y sigue presionado
                if (!nivel_alto) {
                    estado = ESTABLE_BAJO;
                //Si se mantiene en 1 por mas de 20ms, se solto limpiamente
                } else if (absolute_time_diff_us(t_cambio, get_absolute_time()) > VENTANA_MS * 1000) {
                    estado = ESTABLE_ALTO;
                    //Se vuelve a invertir el estado del LED al soltar
                    gpio_xor_mask(1u << LED);
                }
                break;
        }
    }
}