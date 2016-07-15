#include "uart.h"
#include <stdio.h>
#include <util/delay.h>
void sendChar(uint8_t c);
void setperiod();
void Reset();
void SelectExperiment();
void Prompt();
void updatePWM();
void InitPWM();
void setupTimer0();
void setupTimer3();
void clearTimer0();
void clearTimer3();
void clearPWM();
void call();
uint32_t convert();
void sendErrorMessage();
void experiment1();
void experiment2();
void print();
void clearcount();

volatile uint8_t TX_BUFFER[MAX_BUFFER_SIZE];
volatile uint8_t RX_BUFFER[MAX_BUFFER_SIZE];

uint8_t s2[]="Set the period of the GREEN LED Task \n\r";
volatile uart_fifo txFIFO;
volatile uart_fifo rxFIFO;
int volatile jitter_led_task;
long volatile brightness;
int volatile respond;
int volatile finish;
int volatile task;
int volatile j;
int runexp1;
int runexp2;
int runexp3;
int runexp4;
int runexp5;
int runexp6;
int runexp7;
int runexp8;
uint16_t period;
uint16_t store;

int volatile receiveR; //if in collecting the # process
int volatile receiveE; //if in collecting the # process
uint8_t receivebuffer[10]; //The butter that receives the data
uint8_t receivebuffer2[3]; //The butter that receives the data
uint32_t volatile Nindex;
int volatile go;
uint32_t parameter;
int myintvar2;

ISR(USART1_RX_vect) {
	clearTimer0();
    PORTB &= ~(1 << PORTB4);
	clearTimer3();
    PORTD &= ~(1 << PORTD6);
	clearPWM();
	rxFIFO.buffer[rxFIFO.back++] = UDR1;
	rxFIFO.back &= (MAX_BUFFER_SIZE - 1);	// wrap around back to 0
	rxFIFO.length++;
    
	if(receiveR==1) //r is selected in menu option
	{   
		receivebuffer[Nindex] = rxFIFO.buffer[rxFIFO.front++];
		if((receivebuffer[Nindex])!=13&&go==0)
		{		
		  Nindex++;
		}
		else if(go==0)
		{
			uint8_t s4[]="Finished entering number: ";
			sendString(s4);
			sscanf(receivebuffer, "%d", &myintvar2);
			sprintf(receivebuffer, "%d", myintvar2);
			sendString(receivebuffer);
			uint8_t s5[]= "\n\rPlease press g to continue\n\r";
			sendString(s5);
			go=1;
		}
		else if(receivebuffer[Nindex]=='g') //if g is typed
		{
				receiveR=0;
				InitPWM();
				updatePWM();
				setupTimer0(); //1000Hz Timer using Timer 0
				setupTimer3(); //40Hz Timer using Timer 3
				Nindex=0;
				go=0;
				finish=1;
				call();
		}
	}
	else if(receiveE==1) //e is selected in menu option
		{
	       receivebuffer2[Nindex] = rxFIFO.buffer[rxFIFO.front++];
	       Nindex++;
	       if(go==0)
	      {
		   //updatePWM();
		   //Nindex=0;
		   uint8_t s6[]="Selected Experiment: ";
		   sendString(s6);
		   sscanf(receivebuffer2, "%d", &myintvar2);
		   sprintf(receivebuffer2, "%d", myintvar2);
		   sendString(receivebuffer2);
		   uint8_t s5[]= "\n\rPlease press g to continue\n\r";
		   sendString(s5);
		   go=1;
	      }
	       else if(receivebuffer2[Nindex-1]=='g') //if g is typed
	      {
			switch(receivebuffer2[Nindex-2])
			  {
				  case'1': experiment1();break;
				  case'2': experiment2();break;
				  case'3': clearcount();runexp3=1;InitPWM();setupTimer0();setupTimer3();break;
				  case'4': clearcount();runexp4=1;InitPWM();setupTimer0();setupTimer3();break;
				  case'5': clearcount();runexp5=1;InitPWM();setupTimer0();setupTimer3();break;
				  case'6': clearcount();runexp6=1;InitPWM();setupTimer0();setupTimer3();break;
				  case'7': clearcount();runexp7=1;InitPWM();setupTimer0();setupTimer3();break;
				  case'8': clearcount();runexp8=1;InitPWM();setupTimer0();setupTimer3();break;
				  
				  default : sendErrorMessage(); _delay_ms(100); call(); break;
			  }
		    receiveE=0;
		    Nindex=0;
		    go=0;
			finish=1;
	      }
		}
	else
	{
	// toggle Red/Green LED based on character received
	switch(rxFIFO.buffer[rxFIFO.front++]) {		
		case 'p': print();finish=1;break;//
		case 'e': SelectExperiment(); break;
		case 'r': setperiod(); break;
		case 'z': Reset();break;
		default : sendErrorMessage(); respond=1; finish=1; break;
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


//Returns the current input

