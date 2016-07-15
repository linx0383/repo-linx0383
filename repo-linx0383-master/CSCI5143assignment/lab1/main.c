/*
 * lab1.c
 *
 * Created: 2016/2/9 19:07:42
 * Author : Tianshu Lin
 */ 
#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include "uart.h"
#include <avr/wdt.h>
#include "image40.h"
#include <math.h>

#include "houghGray.h"

volatile uint8_t TX_BUFFER[MAX_BUFFER_SIZE];
volatile uint8_t RX_BUFFER[MAX_BUFFER_SIZE];

volatile uart_fifo txFIFO;
volatile uart_fifo rxFIFO;


long volatile time_ms;
long volatile time_to_toggle;
long volatile time_40hz;
long volatile time_1000hz;
long volatile expected_count_green;
long volatile expected_count_red;
long volatile expected_count_yellow;
long volatile count_green;
long volatile count_red;
long volatile count_yellow;
long volatile missed_red;
long volatile missed_green;
long volatile missed_yellow;
long volatile missed_hough;
long volatile missed_jitter;
int volatile jitter_led_task;
int volatile hough_task;
long volatile num;//random number for jitter LED task
long ii;//delay index in exp1
//int volatile k;
int show; //display the menu after pressing button A
//int volatile respond; //if in menu option keeps showing (0 showing, 1 stops)
int volatile finish;
int volatile receiveE; //if in collecting the # process
int volatile receiveR; //if in collecting the # process
long volatile i;
int volatile j;
int volatile a=4;
int runexp1;
int runexp2;
int runexp3;
int runexp4;
int runexp5;
int runexp6;
int runexp7;
int runexp8;
uint32_t volatile time_to_40hztoggle;
uint32_t volatile time_to_10hztoggle=99;
int volatile b; //Recognize the release of button A
int volatile menu; //Judge if prompting the menu option
int volatile task; //if menu option keeps showing after the first option
uint32_t volatile Nindex; // The index for receivebuffer
char receivebuffer[10];
char receivebuffer2[1];
uint8_t s[] = "\n\rPlease select your menu option\n\rp : Print data collected for experiments   e # : Set-Up this experiment number\n\rr # : Set the period of the GREEN LED Task z : Reset all variables for a new experiment\n\rg : Go signal for start experiment\n\r\n";

void call()
{
	sendString(s);
	_delay_ms(50);
}

void clearcount()
{
		time_ms=0;
		time_to_toggle=0;
		time_40hz=0;
		time_1000hz=0;
		time_to_40hztoggle=0;
		time_to_10hztoggle=99;
		count_green=0;
		runexp1=0;
		runexp2=0;
		runexp3=0;
		runexp4=0;
		runexp5=0;
		runexp6=0;
		runexp7=0;
		runexp8=0;
		
		expected_count_green=0;
		expected_count_red=0;
		expected_count_yellow=0;
		count_green=0;
		count_red=0;
		count_yellow=0;
		missed_red=0;
		missed_green=0;
		missed_yellow=0;
		missed_hough=0;
		missed_jitter=0;
}
	
void Reset()
	{
    cli(); 
	wdt_enable(WDTO_15MS); //wd on,15ms
    while(1); //loop
	}


uint32_t convert(uint8_t *s)
{
	uint32_t F=0;
	uint32_t PWM_Value;
	uint32_t bit;
	for(bit=0;bit<=(Nindex-1);bit++)
	{
		F=F+pow(10,(Nindex-1-bit))*(s[bit]-48);
	}
	PWM_Value = (1000*F)/32 - 1;
	return PWM_Value;
}
void SelectExperiment()
{
		//respond=1;
		uint8_t s3[]="Please select the experiment number from 1 to 8 \n\r";
		sendString(s3);
		receiveE=1;
}

void setperiod()
{
	  //respond=1;
	  uint8_t s2[]="Set the period of the GREEN LED Task, then press Enter \n\r";
	  sendString(s2);
	  receiveR=1;
	
}

void setupPin()
{
		DDRB |= (1 << DDB5); //make pin PB5 an output
		DDRB |= (1 << DDB6); //make pin PB6 an GPIO green LED output
		DDRD |= (1 << DDD6); //make pin PD6 an GPIO yellow LED ouput
		DDRB |= (1 << DDB4); //make pin PB4 an GPIO red LED output 
		DDRC |= (1 << DDC7); //make pin PC7 an onboard yellow LED output
		DDRD |= (1 << DDD5); //make pin PD5 an onboard green LED output
		DDRD |= (1 << DDD7); //make pin PD7 an GPIO LED output £¨For test only)
		DDRB &= ~(1 << DDB3); //set Button A as Input
		PORTB |= 1 << PORTB3; //Set Pull_Up Resistor for Button A
		
		USBCON = 0;	// don't have to reset chip after flashing
		DDRB |= (1 << DDB0);	// red led
}

void setupTimer0()
{
	TCCR0B |= (1<< CS01)|(1<< CS00);//setup of prescaler of 64
	TCCR0A |= (1<<WGM01); // setup of CTC
	TIMSK0 |= (1 << OCIE0A);  // enabled global and timer compare interrupt;
	OCR0A = 249; //1000Hz(1ms) 16M/(64*(1+249))=1000Hz
}
void clearTimer0()
{
	TCCR0B = '\0';
	TCCR0A = '\0'; 
	TIMSK0 = '\0'; 
	OCR0A  = '\0';
	TCNT0  = '\0';
}

void InitPWM()
{
	TIMSK1 |= (1<<OCIE1B)|(1<<OCIE1A); //enable timer compare interrupt
	TCCR1A |= (1<<COM1B1)|(1<<WGM11)|(1<<WGM10);// COM1B1=1,COM1B0=0,"Clear OC1B on COMPARE MATCH"; SET AT TOP;WGM11=1,WGM10=1 FAST PWM,OCR1A TOP
	TCCR1B |= (1<<WGM13)|(1<<WGM12)|(1<<CS12)|(1<<CS10);// WGM13=1,WGM12=1 {CS12,CS11,CS10}=101, Prescaler=1024
}
void clearPWM()
{
	TIMSK1 ='\0';
	TCCR1A ='\0';
	TCCR1B ='\0';
	TCNT1 = '\0';
}

void updatePWM()
{
		OCR1A = convert(receivebuffer);
		OCR1B = convert(receivebuffer)/2;
}
void setupTimer3()
{	   // set up timer with prescaler = 64 and CTC mode
	   TCCR3B |= (1 << WGM32)|(1 << CS31)|(1 << CS30);
	   // initialize compare value
	  // OCR3A = 6249; //40Hz(25ms) 16M/(64*(1+6249))=40Hz
	   // enable interrupts
	   TIMSK3 |= (1 << OCIE3A); // enabled timer compare interrupt;
	   TCCR3A = 0x00; // normal operation
}

void clearTimer3()
{
		TCCR3B = '\0';
		TIMSK3 = '\0';
		TCCR3A = '\0';
		TCNT3='\0';
}

void experiment1()
{
	clearcount();
	//PWM part
	OCR1A = 15624;
	OCR1B = 7812;
	
	//OCR0A=1249;
	OCR3A=31249;
	runexp1=1;
	InitPWM();
	setupTimer0(); //1000Hz Timer using Timer 0
	setupTimer3(); //40Hz Timer using Timer 3
}

void experiment2()
{
	clearcount();
	runexp2=1;
	OCR0A = 249; //1000Hz(1ms) 16M/(64*(1+249))=1000Hz
	OCR3A = 6249; //40Hz(25ms) 16M/(64*(1+6249))=40Hz
	OCR1A = 3124;// f=16000000/(1024(3124+1))=5Hz
	OCR1B = 1562;
	InitPWM();
	setupTimer0(); //1000Hz Timer using Timer 0
	setupTimer3(); //40Hz Timer using Timer 3
}

ISR(TIMER0_COMPA_vect)
{
	time_ms++;
	if(time_ms%100==0||time_ms==0)
	{
		expected_count_red++;
	}
	if(time_ms>time_to_10hztoggle) //100ms (10Hz)
	{
		time_to_10hztoggle=time_ms+99;
		hough_task=1;
		missed_hough++;
	}
}
ISR(TIMER1_COMPA_vect) //100ms 10Hz green LED
{
	count_green++; //Maintain a count of the number of toggles of the GREEN LED
	if(runexp2)
	{
		_delay_ms(20);//20ms delay
	}
	if(runexp4)
	{
		_delay_ms(30);
	}
	if(runexp6)
	{
		_delay_ms(105);
	}
}

ISR(TIMER1_COMPB_vect)
{
	count_green++; //Maintain a count of the number of toggles of the GREEN LED
}

ISR(TIMER3_COMPA_vect) //25ms 40Hz yellow LED
{
	if(runexp8)
	{
		sei();
	}
	    time_40hz++; //increase for each 25ms
		
	    if(time_40hz>time_to_40hztoggle)
	    {
	        PORTD ^= (1 << PORTD6); //Toggle GPIO yellow LED
			count_yellow++;
			if(time_40hz>(time_to_40hztoggle+1))
			missed_yellow++;
		    time_to_40hztoggle=(time_40hz+3);
		}
				if(runexp3)
				{
					_delay_ms(20);
				}
				if(runexp5)
				{
					_delay_ms(30);
				}
				if(runexp7)
				{
					_delay_ms(105);
				}
				if(runexp8)
				{
					_delay_ms(105);
				}
		
		num=(rand()%5);
		if(num==4)
		  {
			jitter_led_task=1;
			missed_jitter++;
		  }
}

ISR(PCINT0_vect)
{
	b++;
	if(b==2) //when buttonA gets released
	{   
		setupUART();
		clearTimer0();
		PORTB &= ~(1 << PORTB4);
		clearTimer3();
		PORTD &= ~(1 << PORTD6);
		clearPWM();
		PORTB &= ~(1 << PORTB6);
		runexp1=0;
		while(finish==0)
		{
			//if(respond==0)
			//{
				if(show==0)
				{
                    call();//display the menu option
					show=1;
				}		
			//}
		}
		show=0;
		b=0;
		finish=0;
		//respond=0; //can show up menu option next time to press button A
		InitPWM();	
		setupTimer0(); //1000Hz Timer using Timer 0
		setupTimer3(); //40Hz Timer using Timer 3
	}
}

void setupPinInterrupt()
{
	PCICR |= (1 << PCIE0); //Set Pin Change Interrupt Control Register. pin change interrupt 0 is enabled. Any change on any enabled PCINT7..0 pin will cause an interrupt
	PCMSK0 |= (1 << PCINT3);
}

void print()
{   
	char data[100];
	sprintf(data, "The expected number of toggle of red LED is: %ld\n\r", expected_count_red);
	sendString(data); _delay_ms(50);
    sprintf(data, "The number of toggle of red LED is: %ld\n\r", count_red);
	sendString(data); _delay_ms(50);
	sprintf(data, "The number of missed deadline of red LED is: %ld\n\r", missed_red);
	sendString(data);_delay_ms(50);
	sprintf(data, "The number of toggle of green LED is: %ld\n\r", count_green);
	sendString(data);_delay_ms(50);
	sprintf(data, "The number of toggle of yellow LED is: %ld\n\r", count_yellow);
	sendString(data);_delay_ms(50);
    sprintf(data, "The number of missed deadline of yellow LED is: %ld\n\r", missed_yellow);
	sendString(data);_delay_ms(50);
	sprintf(data, "The number of missed deadline of jitter LED is: %ld\n\r", missed_jitter);
	sendString(data);_delay_ms(50);
	sprintf(data, "The number of missed deadline of hough transform is: %ld\n\n\r", missed_hough);
	sendString(data);_delay_ms(50);
	InitPWM();
	setupTimer0(); //1000Hz Timer using Timer 0
	setupTimer3(); //40Hz Timer using Timer 3
}

void sendErrorMessage() {
	char error[]="Error while parsing the command specified, please try again...\n\r";
	sendString(error);
	InitPWM();
	setupTimer0(); //1000Hz Timer using Timer 0
	setupTimer3(); //40Hz Timer using Timer 3
}


int main(void)
{
	setupPin(); //pin configuration
	PORTD|= (1<<PORTD6);
	PORTB|= (1<<PORTB6);
	PORTB|= (1<<PORTB4);
	_delay_ms(500);
	PORTD&= ~(1<<PORTD6);
	PORTB&= ~(1<<PORTB6);
	PORTB&= ~(1<<PORTB4);
	_delay_ms(500);
	setupTimer0(); //1000Hz Timer using Timer 0
    OCR0A = 249; //1000Hz(1ms) 16M/(64*(1+249))=1000Hz
	setupTimer3(); //40Hz Timer using Timer 3
	OCR3A = 6249; //40Hz(25ms) 16M/(64*(1+6249))=40Hz
	setupPinInterrupt();
	InitPWM(); //PWM using Timer 1
	OCR1A = 3124;// f=16000000/(1024(3124+1))=5Hz
	OCR1B = 1562;// 10Hz
	sei(); // enable global interrupt
	while(1)
		{
			cli();
			if(runexp1==0)
			{
				if(time_ms>time_to_toggle)
			    {
					PORTB ^= (1 << PORTB4); //toggle GPIO red LED
					count_red++;
					if(time_ms>(time_to_toggle+1))
					{
					missed_red++;
					}
					time_to_toggle=(time_ms+99);
			    }
			}
			else
				if(time_ms>time_to_toggle)
				{
					PORTB ^= (1 << PORTB4); //toggle GPIO red LED
					count_red++;
					if(time_ms>(time_to_toggle+1))
					{
					missed_red++;
					}
					time_to_toggle=(time_ms+499);
				}
			sei();
			
			if(jitter_led_task==1)
			    {
			       PORTC |= (1 << PORTC7); //Turn on on board yellow LED
			       for(i=0;i<2400;i++); //5ms
			       PORTC &= ~(1 << PORTC7); //Turn off on board yellow LED
				   missed_jitter--;
			       jitter_led_task=0;
			    }
			if (hough_task == 1)
				{
					for( i=0; i<23; i++) 
					{
						houghTransform( (uint16_t) &red, (uint16_t) &green, (uint16_t) &blue );
					} //takes approximately 50ms
					missed_hough--;
					hough_task=0;
				}
		}
}

