#include "pico/stdlib.h"
//Definimos el pin fisico GP2 para el Logic Analyzer
#define PIN_PRUEBA 2
//Creacion de mascara de bits en tiempo de compilacion
#define LED_MASK (1u << PIN_PRUEBA)
int main() {
//Inicializacion del pin
gpio_init(PIN_PRUEBA);
//Configuracion como salida escribiendo en el registro Output Enable
sio_hw->gpio_oe_set = LED_MASK;
//Ciclo infinito
while (true) {
//Configuracion en alto escribiendo en el registro de Set
sio_hw->gpio_set 
sleep_ms(500);
//Configuracion en bajo escribiendo en el registro de Clear
sio_hw->gpio_clr = LED_MASK;
sleep_ms(500);
}
}
