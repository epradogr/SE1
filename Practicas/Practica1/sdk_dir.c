#include <stdio.h>
#include "pico/stdlib.h"
int main() {
//Inicializacion del pin usando el SDK
gpio_init(PICO_DEFAULT_LED_PIN);
//Configuracion del pin como salida
gpio_set_dir(PICO_DEFAULT_LED_PIN, true);
//Ciclo infinito
while(true) {
//Configuracion en alto usando la funcion del SDK
gpio_put(PICO_DEFAULT_LED_PIN, 1);
sleep_ms(500);
//Configuracion en bajo usando la funcion del SDK
gpio_put(PICO_DEFAULT_LED_PIN, 0);
sleep_ms(500);
}
}
