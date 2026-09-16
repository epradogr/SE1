#include "pico/stdlib.h"
//Definimos el pin fisico GP2 para la simulacion
#define PIN_PRUEBA 2
//Creacion de mascara de bits en tiempo de compilacion
#define LED_MASK (1u << PIN_PRUEBA)
int main() {
//Inicializacion del pin
gpio_init(PIN_PRUEBA);
//Configuracion como salida escribiendo en el registro Output Enable (OE)
sio_hw->gpio_oe_set = LED_MASK;
//Ciclo infinito
while (true) {
//Inversion de estado usando el registro especial XOR (Toggle)
sio_hw->gpio_togl = LED_MASK;
sleep_ms(500);
}
}