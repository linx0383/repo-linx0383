#include "uart.h"
#include <stdio.h>
#include <util/delay.h>
#include <math.h>
void sendChar(uint8_t c);
void setReferenceV();

volatile uint8_t TX_BUFFER[MAX_BUFFER_SIZE];
volatile uint8_t RX_BUFFER[MAX_BUFFER_SIZE];

volatile uart_fifo txFIFO;
volatile uart_fifo rxFIFO;
signed int ref1,ref2;
long volatile encoder_count;
static int global_counts_m2;
uint8_t receivebuffer[10]; //The butter that receives the data
int receiveR;
int myintvar2,myintvar3;
int button;
long volatile current_value;
int lastvalue,lastvalue_store;
char svalue[20];
float Kp;
float Kd;
long Kp_View,Kd_View;
int experiment;
//float Kp_View,Kd_View;
uint32_t volatile Nindex;
long Refer_Value;
uint32_t Difference1;
uint32_t Difference2;

uint32_t convert(uint8_t *s)
{
	//Refer_Value=0;
	uint32_t bit;
	for(bit=0;bit<Nindex;bit++)
	{
		Refer_Value=Refer_Value+pow(10,(Nindex-1-bit))*(s[bit]-48);
	}
	return Refer_Value;
}

void view()
{
	//Kd, Kp, Vm, Pr, Pm, and T
	Kp_View=Kp*1000;
	sscanf(svalue, "%ld", &Kp_View);
	sprintf(svalue, "Kp is %ld/1000\r\n", Kp_View);
	sendString(svalue);
	Kd_View=Kd*1000;
    sscanf(svalue, "%ld", &Kd_View);
	sprintf(svalue, "Kd is %ld/1000\r\n", Kd_View);
	sendString(svalue);
	
	sscanf(svalue, "%ld", &Refer_Value);
	sprintf(svalue, "Pr is %ld\r\n", Refer_Value);
	sendString(svalue);

	sscanf(svalue, "%ld", &current_value);
	sprintf(svalue, "Pm is %ld\r\n", current_value);
	sendString(svalue);
	
	sscanf(svalue, "%d", &OCR1B);
	sprintf(svalue, "T is %d\r\n", OCR1B);
	sendString(svalue);
}

void runtra()
{
	if(Refer_Value>=lastvalue)
	{
		PORTE &= ~(1 << PORTE2);//rotate forward
		DDRB |= (1 << DDB6);
	}
	else
	{
		PORTE |= (1 << PORTE2); //rotate backward
		DDRB |= (1 << DDB6); //start rotating
	}
	button=1;
}


ISR(USART1_RX_vect) {
	rxFIFO.buffer[rxFIFO.back++] = UDR1;
	rxFIFO.back &= (MAX_BUFFER_SIZE - 1);	// wrap around back to 0
	rxFIFO.length++;
	
	if(receiveR==1) //r is selected in menu option
	{
		receivebuffer[Nindex] = rxFIFO.buffer[rxFIFO.front++];
		if((receivebuffer[Nindex])!=13)
		{
			Nindex++;
		}
		else
		{
			uint8_t s4[]="\r\nFinished entering reference count: ";
			//_delay_ms(1000);
			sendString(s4);
			sscanf(receivebuffer, "%d", &myintvar2);
			sprintf(receivebuffer, "%d\r\n", myintvar2);
			sendString(receivebuffer);
			Refer_Value=0;
			Refer_Value=convert(receivebuffer);
			sscanf(svalue, "%ld", &Refer_Value);
			sprintf(svalue, "Refer_Value is %ld\r\n", Refer_Value);
			sendString(svalue);
			encoder_count = global_counts_m2;
			sscanf(svalue, "%d", &lastvalue);
			sprintf(svalue, "Last Count(Measured) is %d\r\n", lastvalue);
			sendString(svalue);
			lastvalue_store=lastvalue;
			Nindex=0;
			receiveR=0;
		}
	}
	else
	{
	// toggle Red/Green LED based on character received
	switch(rxFIFO.buffer[rxFIFO.front++]) {
		case 'R':setReferenceV();break;
		case 'r':setReferenceV(); break;
		case 'P': Kp=Kp+0.01;break; 
		case 'p': Kp=Kp-0.01;break;
		case 'D': Kd=Kd+0.01;break;
		case 'd': Kd=Kd-0.01;break;
		case 'V':view();break;
		case 'v':view();break;
		case 't':runtra();break;
		case 'e':experiment=1;break;
        default: break;
	}
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
	while((UCSR1A & (1<<UDRE1)) == 0);	// wait while data register is NOT empty
	UDR1 = c;
}
