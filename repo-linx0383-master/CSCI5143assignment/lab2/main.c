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
#include <math.h>
#include "uart.h"
long volatile encoder_count;
uint8_t receivebuffer[10]; //The butter that receives the data
int button;
long volatile current_value;
long volatile previous_value;
int lastvalue;
int lastvalue_store;
char s[20];
float Kp=0.4;
float Kd=0.2;
int receiveR;
int experiment,experiment2,experiment3;
long Refer_Value=0;
char svalue[20];

static int global_counts_m2; //current count (measured value)

static char global_error_m2;

static char global_last_m2a_val;
static char global_last_m2b_val;

uint8_t msg[] = "Please select your menu option\n\rR/r : Set the reference position\n\rP: Increase Kp by an amount*\n\rp: Decrease Kp by an amount\n\rD: Increase Kd by an amount\n\rd: Decrease Kd by an amount\n\rV/v: View the current values\n\rt: Execute trajectory\n\r\n";
void call()
{
	sendString(msg);
	_delay_ms(100);
}

void setReferenceV()
{
	uint8_t msg2[]="Set the reference value, then press Enter \n\r";
	sendString(msg2);
	receiveR=1;
}

void setupPin()
{
	DDRB &= ~(1 << DDB5); //make pin PB5 as input Channel B
	DDRB &= ~(1 << DDB4); //make pin PB4 as input Channel A
	//DDRB |= (1 << DDB6); //motor 2 PWM
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
		OCR1A = 99;// f=16000000/64(99+1)=2500Hz
		OCR1B = 0; // for 4, 50000Hz

}

void setupTimer3()
{	   // set up timer with prescaler = 64 and CTC mode
	TCCR3B |= (1 << WGM32)|(1 << CS31)|(1 << CS30);
	// initialize compare value
	 OCR3A = 6249; //40Hz(25ms) 16M/(64*(1+6249))=40Hz
	 //OCR3A = 12499; //20Hz(0.05s) 16M/(64*(1+12499))=20Hz
	// enable interrupts
	TIMSK3 |= (1 << OCIE3A); // enabled timer compare interrupt;
	TCCR3A = 0x00; // normal operation
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

ISR(TIMER3_COMPA_vect) //20ms 50Hz
{
   if(button==1)
   {	
		OCR1B=Kp*abs((Refer_Value-current_value))-Kd*abs((previous_value-current_value));
		//OCR1B=Kp*abs(Refer_Value-current_value);
		//OCR1B=25;
		if(OCR1B<3)
		 {
		   OCR1B=3;
		 }
		 if(OCR1B>75)
		 {
		   OCR1B=75;
		 }
			if(global_counts_m2<Refer_Value)
			{
				PORTE &= ~(1 << PORTE2);
			}
			else if(global_counts_m2>Refer_Value)
			{
				PORTE |= (1 << PORTE2);
			}
			else if(global_counts_m2==Refer_Value)
			{
				DDRB &= ~(1 << DDB6); //stop rotating
				button=0;
				OCR1B=0;
				lastvalue=current_value;
			}
			previous_value=current_value;
			current_value = global_counts_m2;
			//sprintf(s, "%d %d %d\r\n", (int)current_value, Refer_Value, OCR1B);
			sprintf(s, "Pm: %d Pr: %ld ", (int)current_value, Refer_Value);
			sendString(s);
			sscanf(svalue, "%d", &OCR1B);
			sprintf(svalue, "T: %d\r\n", OCR1B);
			sendString(svalue);
	}
	if (experiment==1)
		{
			current_value=0;
			button=1;
			experiment=0;
			Refer_Value=562;
			PORTE &= ~(1 << PORTE2);//rotate forward
			DDRB |= (1 << DDB6);
			experiment2=1;
		}
		if(experiment2==1&&current_value==562)
		{
			_delay_ms(500);
			experiment2=0;
			experiment3=1;
			button=1;
			Refer_Value=-1686;
			PORTE |= (1 << PORTE2);//rotate backward
			DDRB |= (1 << DDB6);
		}
		if(experiment3==1&&current_value==-1686)
		{
		    _delay_ms(500);
		    experiment3=0;
		    button=1;
			Refer_Value=-1655;
			PORTE &= ~(1 << PORTE2);//rotate forward
			DDRB |= (1 << DDB6);
		}
}


int main(void)
{
	setupPin(); //pin configuration
	setupUART();
	setupPinInterrupt();
	setupTimer3();
	InitPWM(); 
	call();
    while(1) 
    {
	}
}

