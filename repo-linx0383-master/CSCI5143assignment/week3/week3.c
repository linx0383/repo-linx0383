// ********************************************************************************
// Includes
// ********************************************************************************
#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

long volatile time_ms;
int flag;
long volatile i;
long volatile time_to_toggle;

// ********************************************************************************
// Interrupt Routines
// ********************************************************************************
// timer1 overflow
void setupLED()
{
	DDRC |= (1 << DDC7); // Make pin PC7 be an output(yellow on-board LED). Blink at 1Hz.
	DDRD |= (1 << DDD5); // make pin PD5 be an output(green on-board LED). Blink at 4Hz.
	DDRD |= (1 << DDD1); // Make pin PD6 be an output. Red external LED
	DDRD |= (1 << DDD4); // Make pin PB6 be an output. Green external LED
	DDRB |= (1 << DDB5); // make pin PB5 be an output (for testing)
	DDRB |= (1 << DDB0); // make pin PB5 be an output (for testing)
	DDRB &= ~(1 << DDB3); //set Button A as Input
	PORTB |= 1 << PORTB3; //Set Pull_Up Resistor for Button A
}

void setupTimer3()
{	// set up timer with prescaler = 1024 and CTC mode
	TCCR3B |= (1 << WGM12)|(1 << CS32)|(1 << CS30);
	// initialize compare value
	OCR3A = 15624; //1Hz(1s)
	// enable interrupts
	TIMSK3 |= (1 << OCIE3A); // enabled global and timer compare interrupt;
	TCCR3A = 0x00; // normal operation
}

void setupTimer1()
{
	// set up timer with prescaler = 8 and CTC mode
	TCCR1B |= (1 << WGM12)|(1 << CS11);
	// initialize compare value
	OCR1A = 1999; //1000Hz(1ms)
	// enable interrupts
	TIMSK1 |= (1 << OCIE1A); // enabled global and timer compare interrupt;
	TCCR1A = 0x00; // normal operation
	sei(); // enable global interrupt
}

ISR(TIMER1_COMPA_vect) 
{
		time_ms++;
		if ((PINB & (1<<PB3))==0)
		{
			flag=1;
		}
}

ISR(TIMER3_COMPA_vect) {
			PORTC ^= (1 << PORTC7); // Toggle the yellow LED.
}

// ********************************************************************************
// Main
// ********************************************************************************
int main() 
{
	setupLED();
	setupTimer3();
	setupTimer1();
	while(1) 
       {
			if(time_ms>time_to_toggle)
			{
				PORTD ^= (1 << PORTD5); //green on board LED
				time_to_toggle=(time_to_toggle+250);
			}
				if(flag==1)		
			{
				PORTD |= (1 << PORTD1);
				for(i=0;i<144000;i++); //300ms
				if(time_ms>time_to_toggle)
					{
						PORTD ^= (1 << PORTD5); //green on board LED
						time_to_toggle=(time_to_toggle+250);
					}				
				PORTD &= ~(1 << PORTD1);
				PORTD |= (1 << PORTD4);
				for(i=0;i<240000;i++); //500ms
				if(time_ms>time_to_toggle)
					{
						PORTD ^= (1 << PORTD5); //green on board LED
						time_to_toggle=(time_to_toggle+250);
					}
				PORTD &= ~(1 << PORTD4);
				PORTD |= (1 << PORTD1);
				for(i=0;i<192000;i++); //400ms
				if(time_ms>time_to_toggle)
					{
						PORTD ^= (1 << PORTD5); //green on board LED
						time_to_toggle=(time_to_toggle+250);
					}	
				PORTD &= ~(1 << PORTD1);
				flag=0;	
            }
	   }
}