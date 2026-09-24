#include "pico/stdlib.h"      
#include "hardware/uart.h"    
#include "hardware/irq.h"     // Manejo de interrupciones
#include <stdio.h>            // sprintf, sscanf
#include <string.h>           // strncmp

#define UART_ID uart0         // Instancia UART0 del RP2040
#define BAUD_RATE 115200      // Velocidad nominal en bits por segundo
#define TX_PIN 0              // GP0 = TX
#define RX_PIN 1              // GP1 = RX
#define LED_PIN 25            // LED integrado de la Pico

// Tamaño de los buffers circulares
#define BUF_SIZE 256
#define MASK (BUF_SIZE - 1)


// La ISR escribe en rx_head; el main lee desde rx_tail.
volatile uint8_t rx_buffer[BUF_SIZE];
// Buffer vacío: head == tail. "volatile" porque lo modifica una interrupción.
volatile uint16_t rx_head = 0, rx_tail = 0;
volatile uint32_t descartados = 0;   // Bytes perdidos por buffer RX lleno


// Aquí el main escribe en tx_head; la ISR lee desde tx_tail.
volatile uint8_t tx_buffer[BUF_SIZE];
volatile uint16_t tx_head = 0, tx_tail = 0;


volatile uint32_t hw_overruns = 0;         // FIFO del hardware se desbordó
volatile uint32_t hw_framing_errors = 0;   // Bit de parada inválido 

uint32_t periodo_led = 500000;  

// Encola un caracter
void enviar_char(char c) {
  
    irq_set_enabled(UART0_IRQ, false);

    uint16_t siguiente = (tx_head + 1) & MASK;   // Posición siguiente 
    if (siguiente != tx_tail) {                  // Si no está llena la cola guardamos el carácter
        tx_buffer[tx_head] = c;                  
        tx_head = siguiente;
    }                                            // Si está llena, el carácter se pierde

    irq_set_enabled(UART0_IRQ, true);
    // Activamos la interrupción de TX 
    uart_set_irq_enables(UART_ID, true, true);
}

// Encola una cadena completa
void enviar_texto(const char* str) {
    while (*str) {
        enviar_char(*str++);
    }
}


// Se ejecuta automáticamente cuando el UART genera una interrupción.
static void on_uart_irq(void) {
  
    while (uart_is_readable(UART_ID)) {

        // Leemos el registro de estado de errores (RSR)
        uint32_t rsr = uart_get_hw(UART_ID)->rsr;
        if (rsr & UART_UARTDR_OE_BITS) hw_overruns++;         // Overrun
        if (rsr & UART_UARTDR_FE_BITS) hw_framing_errors++;   // Error de trama
        uart_get_hw(UART_ID)->rsr = 0;                        // Limpiamos las banderas

        uint8_t c = uart_getc(UART_ID);                 // Sacamos el byte del FIFO
        uint16_t siguiente = (rx_head + 1) & MASK;

        if (siguiente != rx_tail) {     // Hay espacio: guardamos
            rx_buffer[rx_head] = c;
            rx_head = siguiente;
        } else {
            descartados++;              // Buffer lleno: contamos el byte perdido
        }
    }

  
    if (uart_is_writable(UART_ID)) {
        if (tx_tail != tx_head) {
            // Hay datos en cola: los pasamos al FIFO mientras quepan
            while (uart_is_writable(UART_ID) && tx_tail != tx_head) {
                uart_putc(UART_ID, tx_buffer[tx_tail]);
                tx_tail = (tx_tail + 1) & MASK;
            }
        } else {
            // Cola vacía: apagamos la IRQ de TX
            uart_set_irq_enables(UART_ID, true, false);
        }
    }
}

void procesar_comando(char* linea) {
    // "get stats": responde con los contadores de error
    if (strncmp(linea, "get stats", 9) == 0) {
        char msg[128];
        sprintf(msg, "overruns=%u, framing errors=%u, descartados=%u\r\n",
                (unsigned int)hw_overruns, (unsigned int)hw_framing_errors, (unsigned int)descartados);
        enviar_texto(msg);

    // "set led on": enciende el LED fijo
    } else if (strncmp(linea, "set led on", 10) == 0) {
        periodo_led = 0;          // 0 detiene el parpadeo automático
        gpio_put(LED_PIN, 1);
        enviar_texto("ok\r\n");

    // "set led off": apaga el LED
    } else if (strncmp(linea, "set led off", 11) == 0) {
        periodo_led = 0;
        gpio_put(LED_PIN, 0);
        enviar_texto("ok\r\n");

    // "set periodo N": parpadeo cada N milisegundos
    } else if (strncmp(linea, "set periodo ", 12) == 0) {
        int nuevo_periodo;
        // sscanf devuelve cuántos valores logró leer
        if (sscanf(linea + 12, "%d", &nuevo_periodo) == 1) {
            periodo_led = nuevo_periodo * 1000;   
            enviar_texto("ok\r\n");
        } else {
            enviar_texto("error: valor invalido\r\n");
        }

    } else {
        enviar_texto("error: comando desconocido\r\n");
    }
}


int main() {
    stdio_init_all();   // Inicializa stdio

    // Asignamos la función UART a los pines
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);

    // uart_init devuelve el baud rate REAL logrado 
    uint real_baud = uart_init(UART_ID, BAUD_RATE);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);   // 8N1

    // Registramos nuestra ISR como manejador exclusivo de la IRQ del UART0
    irq_set_exclusive_handler(UART0_IRQ, on_uart_irq);
    irq_set_enabled(UART0_IRQ, true);
    
    uart_set_irq_enables(UART_ID, true, false);

    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

  
    float error_baud = ((float)(BAUD_RATE - (int)real_baud) / BAUD_RATE) * 100.0f;
    if (error_baud < 0) error_baud = -error_baud;   // Valor absoluto

    char boot_msg[128];
    sprintf(boot_msg, "Baud nominal: %d | Baud real: %u | Error: %.3f%%\r\n", BAUD_RATE, real_baud, error_baud);
    enviar_texto(boot_msg);   // Mensaje de arranque, por la cola TX

    // Marcas de tiempo para las tareas periódicas
    uint32_t t_led = time_us_32();
    uint32_t t_log = time_us_32();

    char linea[64];   // Línea de comando que se va armando
    uint8_t n = 0;    // Cantidad de caracteres acumulados


    while (true) {

        if (periodo_led > 0 && (time_us_32() - t_led >= periodo_led)) {
            t_led = time_us_32();
            gpio_xor_mask(1u << LED_PIN);   // Invierte el estado del LED
        }

        //  mensaje de log cada 1 segundo 
        if (time_us_32() - t_log >= 1000000) {
            t_log = time_us_32();
            enviar_texto("[Log] Tarea ejecutandose...\r\n");   // Va a la cola, no bloquea
        }

        //  procesar lo recibido
        while (rx_tail != rx_head) {
            char c = rx_buffer[rx_tail];
            rx_tail = (rx_tail + 1) & MASK;

            if (c == '\r' || c == '\n') {
                // Enter: fin de comando
                enviar_texto("\r\n");     // Salto de línea limpio en la terminal
                linea[n] = '\0';          // Cerramos la cadena
                if (n > 0) {
                    procesar_comando(linea);   // Solo si escribió algo
                }
                n = 0;                    // Reiniciamos para el siguiente comando
            } else if (n < sizeof(linea) - 1) {  
                enviar_char(c);           // Eco: el usuario ve lo que escribe
                linea[n++] = c;
            }
        }
    }
}