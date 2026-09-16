#include <stdio.h>
#include "pico/stdlib.h"

/* gpio_xor_mask(1u << PICO_DEFAULT_LED_PIN) es donde se crea
la marca de bits en la compilación
*/


/*código blink * tonggle usando SDK
*/
int main() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, true);


    /* ciclo infinito*/
    while(true){

        /* usamos xor (tonggle)
        que es el registro especial para inversion*/

        gpio_xor_mask(1u << PICO_DEFAULT_LED_PIN);
        sleep_ms(500);
    }
}