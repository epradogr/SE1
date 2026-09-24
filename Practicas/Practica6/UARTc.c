#include "pico/stdlib.h"      // Funciones básicas del SDK de la Pico (GPIO, tiempos, stdio)
#include "hardware/uart.h"    // API para controlar los periféricos UART por hardware
#include <stdio.h>            // getchar, putchar

#define UART_ID uart0         // Usamos la instancia UART0 del RP2040
#define BAUD_RATE 115200// Velocidad de comunicación: 115200 bits por segundo


int main() {
    
    stdio_init_all();

    // Asigna la función UART a los pines GP0 (TX) y GP1 (RX).
    // Por defecto los pines son GPIO normales; hay que cambiar su función.
    gpio_set_function(0, GPIO_FUNC_UART);   // GP0 = UART0 TX (transmisión)
    gpio_set_function(1, GPIO_FUNC_UART);   // GP1 = UART0 RX (recepción)

    // Inicializa el UART0 a la velocidad indicada
    uart_init(UART_ID, BAUD_RATE);

    // Formato de trama: 8 bits de datos, 1 bit de parada, sin paridad 
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

  
    while (true) {
       
        // Intenta leer un carácter del USB sin bloquear (timeout de 0 µs).
        int c = getchar_timeout_us(0);

        // Si no devolvió PICO_ERROR_TIMEOUT, significa que sí llegó un dato
        if (c != PICO_ERROR_TIMEOUT) {
            uart_putc(UART_ID, (char)c);   // Lo envía por el pin TX del UART
        }

       
        // Si hay un byte esperando en el buffer de recepción del UART
        if (uart_is_readable(UART_ID)) {
            putchar(uart_getc(UART_ID));   //lo lee y lo manda por USB
        }
    }
}