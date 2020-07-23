#include <avr/io.h>
#include "usart.h"

void USART_init(uint16_t baud) {
    baud = 103; //9600 bps
    /* Set baud rate */
    UBRR1H = (baud >> 8);
    UBRR1L = baud;
    /* Enable receiver and disable transmitter */
    UCSR1B = (1 << RXEN1) | (1 << TXEN1);
    /* Enable Receive complete Interrupt */
    UCSR1B |= (1 << RXCIE1);
    /*
     * UCSRnB register should be set along with UCSRnC
     */
    UCSR1B |= (0 << UCSZ12);				// Data size: 8bit

    UCSR1C |= ((0 << UMSEL11) | (0 << UMSEL10));	// Asynchronous USART
    UCSR1C |= ((0 << UPM11) | (0 << UPM10));		// Parity: disabled
    UCSR1C |= (0 << USBS1);				// Stop bit: 1bit
    UCSR1C |= ((1 << UCSZ11) | (1 << UCSZ10));		// Data size: 8bit
}
