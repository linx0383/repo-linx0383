#include "uart.h"

void sendChar(uint8_t c);

volatile uint8_t TX_BUFFER[MAX_BUFFER_SIZE];
volatile uint8_t RX_BUFFER[MAX_BUFFER_SIZE];

volatile uart_fifo txFIFO;
volatile uart_fifo rxFIFO;
int F;
int R;


ISR(USART1_RX_vect) {
	rxFIFO.buffer[rxFIFO.back++] = UDR1;
	rxFIFO.back &= (MAX_BUFFER_SIZE - 1);	// wrap around back to 0
	rxFIFO.length++;

	// toggle Red/Green LED based on character received
	switch(rxFIFO.buffer[rxFIFO.front++]) {
		case 'f': F=1; break;
		case 'r': R=1; break;
        default: break;
	}
	rxFIFO.front &= MAX_BUFFER_SIZE - 1; // wrap around back to 0
}


ISR(USART1_TX_vect) {
	PORTC ^= (1 << PORTC7);	// toggle yellow led
	if(txFIFO.length == 0) {	// buffer is empty so return
		return;
	}
	if(UCSR1A & (1<<UDRE1)) {	// ensure data register is empty
		sendChar(txFIFO.buffer[txFIFO.front++]);
		txFIFO.length--;
		txFIFO.front &= (MAX_BUFFER_SIZE - 1); // wrap around back to 0
	}

}

void bufferInit(volatile uint8_t *buf, volatile uart_fifo *f) {
	f->buffer = buf;
	f->front = 0;
	f->back = 0;
	f->length = 0;
}

/*
Setup UART1 for 57.6k Baud Rate
using transmit and receive interrupts
*/
void setupUART(void) {
	bufferInit(TX_BUFFER, &txFIFO);
	bufferInit(RX_BUFFER, &rxFIFO);

	//UBRR = fosc / (16 * baud) - 1

	UBRR1 = ((F_CPU/(16*57600)) - 1);
	UCSR1C |= (1 << UCSZ11) | (1 << UCSZ10);		// 8 bit char size
	UCSR1B |= (1 << RXCIE1);	// enable receive interrupt
	UCSR1B |= (1 << TXCIE1);	// enable transmit interrupt
	UCSR1B |= (1 << RXEN1);		// enable receive
	UCSR1B |= (1 << TXEN1);		// enable transmit
}

// return -1 if buffer is full
int8_t sendString(uint8_t *s) {
	cli();
	if((txFIFO.length + sizeof(s)) > MAX_BUFFER_SIZE) { return -1; }	// not enough room in buffer

	while(*s != 0x00) {	// put string to send in buffer
		txFIFO.buffer[txFIFO.back++] = *s++;
		txFIFO.length++;
		txFIFO.back &= (MAX_BUFFER_SIZE - 1); // wrap around back to 0
	}

	if(UCSR1A & (1<<UDRE1)) {	// if data register is empty
		sendChar(txFIFO.buffer[txFIFO.front++]);
		txFIFO.length--;
		txFIFO.front &= (MAX_BUFFER_SIZE - 1); // wrap around back to 0
	}
	sei();
	return 0;
}

void sendChar(uint8_t c) {
	UDR1 = c;
}
