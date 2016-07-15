/*
 * HWMotor.c
 *
 * Created: 2016/3/7 15:09:55
 * Author : Tianshu Lin
 */ 
#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include "uart.h"
#define ref1 2249
#define ref2 -2249
long volatile encoder_count;
int F;
int R;
int button; // If either f or r was pressed, button will be 1
int direction = 1;// 0 means moving forward, 1 means moving backward
int setup = 1;
long volatile initial;
char s[20];

static int global_counts_m2;

static char global_error_m2;

static char global_last_m2a_val;
static char global_last_m2b_val;

void setupPin()
{
	DDRB &= ~(1 << DDB5); //make pin PB5 as input Channel B
	DDRB &= ~(1 << DDB4); //make pin PB4 as input Channel A
	DDRB |= (1 << DDB6); //motor 2 PWM
}

void setupPinInterrupt()
{
	  PCICR |= (1 << PCIE0); //Set Pin Change Interrupt Control Register. pin change interrupt 0 is enabled. Any change on any enabled PCINT7..0 pin will cause an interrupt
	  PCMSK0 |= (1 << PCINT4);//Enable PCINT4
	  PCMSK0 |= (1 << PCINT5);//Enable PCINT5
}

void InitPWM()
{
	    
		TCCR1A |= (1<<COM1B1)|(1<<WGM11)|(1<<WGM10);// COM1B1=1,COM1B0=0,"Clear OC1B on COMPARE MATCH"; SET AT TOP;WGM11=1,WGM10=1 FAST PWM,OCR1A TOP
	    TCCR1B |= (1<<WGM13)|(1<<WGM12)|(1<<CS11)|(1<<CS10);// WGM13=1,WGM12=1 {CS12,CS11,CS10}=011, Prescaler=64
		OCR1A = 49;// f=16000000/64(49+1)=5000
		OCR1B = 5;

}

void calibration()
{
	PORTE &= ~(1 << PORTE2);
    while (setup)
    {
	    cli();
	    if (global_counts_m2 != initial)
	    {
		    initial = global_counts_m2;
		    sprintf(s, "%d\r\n", (int)initial);
		    sendString(s);
	    }
	    if ( (global_counts_m2 <= 0) && (direction == 0) )
	    {
		    direction = 1;
		    DDRB &= ~(1 << DDB6);
		    _delay_ms(300);
			setup = 0;
	    }
	    if ( (global_counts_m2 >= ref1) && (direction == 1) )
	    {
		    direction = 0;
		    DDRB &= ~(1 << DDB6);
		   _delay_ms(300);
		    PORTE |= (1 << PORTE2);
		    DDRB |= (1 << DDB6);
	    }
	    sei();
    }	
}

ISR(PCINT0_vect)
{
	/* This code keeps encoder counts for motors 1 and motors 2.
	   Each encoder has 2 channels A and B, both of which cause this ISR
	   to fire when a signal change is experienced on that pin.
	   By comparing the value of the last read of Channel A to the current Channel B value,
	   and comparing the last read of Channel B to the current Channel A value will determine
	   whether encoder counts are increasing or decreasing.
	   
	   Encoder counts are stored in global_counts_m1 and global_counts_m2.
	   These should be global variables accessible by main and other ISRs.
	   Keep in mind that when reading these values in main, they should be protected with cli().
	   To get this functional:
	   - plug in Channel A and Channel B wires to some PCINT pins.
	   - enable those pins as input.
	   - enable the PCINT0 interrupt in PCICR
	   - enable those 2 pin interrupts in PCMSK0
	*/

	// Determine the current value of all channels. 
	// These are pololu functions. Can implement with ((PINx & (1 << PINxn)) >> PINxn
	//unsigned char m1a_val = OrangutanDigital::isInputHigh(global_m1a);
	unsigned char m2a_val = (PINB &(1<<PINB4))>>PINB4;
	//unsigned char m1b_val = OrangutanDigital::isInputHigh(global_m1b);
	unsigned char m2b_val = (PINB &(1<<PINB5))>>PINB5;

	// Determine if the last read of one channel is different from the current read of the other channel
	// which tells you whether you need to add or subtract from encoder count
	char plus_m2 = m2a_val ^ global_last_m2b_val;
	char minus_m2 = m2b_val ^ global_last_m2a_val;

	// add and/or subtract as determined above

	if(plus_m2)
		global_counts_m2 += 1;
	if(minus_m2)
		global_counts_m2 -= 1;

	// do some error checking to see if you missed an interrupt or something is wrong
	if(m2a_val != global_last_m2a_val && m2b_val != global_last_m2b_val)
		global_error_m2 = 1;

	// save state for next interrupt
	global_last_m2a_val = m2a_val;
	global_last_m2b_val = m2b_val;
}


int main(void)
{
    setupUART();
	setupPin(); //pin configuration
	setupPinInterrupt();
	InitPWM(); 
	calibration();
    while (1) 
    {
				if (F == 1)
				{
					F = 0;
					encoder_count = global_counts_m2;
					//sprintf(s, "Moving Forward from Count: %d\r\n", (int)encoder_count );
					//sendString(s);
					PORTE &= ~(1 << PORTE2);
					DDRB |= (1 << DDB6);
					button=1;
				}
				if (R == 1)
				{   
					//calibration2();
					R = 0;
					encoder_count = global_counts_m2;
					//sprintf(s, "Moving Backward from Count: %d\r\n", (int)encoder_count );
					//sendString(s);
					PORTE |= (1 << PORTE2);
					DDRB |= (1 << DDB6);
					button=1;
				}
				cli();			
				if(button==1)
				{
					if (global_counts_m2 != initial)
					{
						initial = global_counts_m2;
						sprintf(s, "%d\r\n", (int)initial);
						sendString(s);
					}
				}
				if ( ((global_counts_m2 - encoder_count) >= ref1) || ((global_counts_m2 - encoder_count) <= ref2) )
				{
					DDRB &= ~(1 << DDB6); //stop rotating
					button=0;
					_delay_ms(200);
				}
				sei();
    }
}

