#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
int a=0; //Global index variable used for judging how many times the button is pressed
void setup()
{
	DDRD |= (1 << DDD5);
	DDRB &= ~(1 << DDB3); //set Button A as Input
	PORTB |= 1 << PORTB3; //Set Pull_Up Resistor for Button A
	PCICR |= (1 << PCIE0); //Set Pin Change Interrupt Control Register. pin change interrupt 0 is enabled. Any change on any enabled PCINT7..0 pin will cause an interrupt
	SREG |= (1 << SREG_I);
	PCMSK0 |= (1 << PCINT3);
	USBCON = 0; //// Disable USB interrupts
}
ISR(PCINT0_vect)
{
 int i;
 long volatile j;
 long volatile k;
 a++;
 if(a==2)
 {
 for (i=0;i<10;i++)
	 {
		 PORTD &= ~(1 << PORTD5); // Turn the LED off.
		 for (j=0;j<60000;j++);
		 PORTD |= (1 << PORTD5); // Turn the LED on.
		 for (k=0;k<60000;k++);
	 }
	 a=0;
 }
 //cli();
}
int main()
{
	setup();
	sei();
	DDRC |= (1 << DDC7); // Make pin 13 be an output.
	while(1)
	{
		PORTC |= (1 << PORTC7); // Turn the LED on.
		_delay_ms(1000);
		PORTC &= ~(1 << PORTC7); // Turn the LED off.
	    _delay_ms(1000);
	}
}