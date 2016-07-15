// Counts number of button presses
// output count to serial
// blink a led according to count

const int yellowbutton = 17;                    // yellow button is connected to pin 17
const int greenbutton = 14;
const int yellowledPin = 13;                      // led on pin 13
int yellowbuttonPresses = 0;                // how many times the button has been pressed 
int greenbuttonPresses = 0;                // how many times the button has been pressed 
int lastyellowPressCount = 0;               // to keep track of last press count

int yellowledstate = LOW;             // yellowledstate used to set the LED
int greenledstate = LOW;

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long yellowpreviousMillis = 0;        // will store last time LED was updated
unsigned long greenpreviousMillis = 0;        // will store last time LED was updated

// constants won't change :
const long yellowinterval = 1250;           // yellowinterval at which to blink (milliseconds)
const long greeninterval = 250;           // yellowinterval at which to blink (milliseconds)

void yellowsetup() {
  pinMode(yellowbutton, INPUT);          // Set the yellow button pin as input
  digitalWrite(yellowbutton, HIGH);
}

void greensetup() {
  DDRD |= (1 << DDD5);
  pinMode(greenbutton,INPUT_PULLUP);
}

void setup(){
  yellowsetup();
  greensetup();
}

void loopyellow(){
  if (digitalRead(yellowbutton) == LOW)  // check if button C was pressed
  {
    yellowbuttonPresses++;                  // increment yellowbuttonPresses count
    delay(400);                       // debounce switch
  }
  if (yellowbuttonPresses == 3) yellowbuttonPresses = 0;         // rollover every third press
    if (yellowbuttonPresses == 1){
      digitalWrite(yellowledPin, HIGH);
      }
    if (yellowbuttonPresses == 2){
        unsigned long yellowcurrentMillis = millis();

  if (yellowcurrentMillis - yellowpreviousMillis >= yellowinterval) {
    // save the last time you blinked the yellow LED
    yellowpreviousMillis = yellowcurrentMillis;

    // if the LED is off turn it on and vice-versa:
    if (yellowledstate == LOW) {
      yellowledstate = HIGH;
    } else {
      yellowledstate = LOW;
    }
    // set the LED with the yellowledstate of the variable:
    digitalWrite(yellowledPin, yellowledstate);                     // wait again
    }
    }
    if ((yellowbuttonPresses == 3)||(yellowbuttonPresses == 0)){
      digitalWrite(yellowledPin, LOW);       // turn off led
    }
    lastyellowPressCount = yellowbuttonPresses;    // track last press count
}

void loopgreen(){
  if (digitalRead(greenbutton) == LOW)  // check if button A was pressed
  {
    greenbuttonPresses++;                  // increment greenbuttonPresses count
    delay(400);                       // debounce switch
  }
  if (greenbuttonPresses == 3) greenbuttonPresses = 0; 
  if (greenbuttonPresses == 1){
  PORTD &= ~(1 << PORTD5); // Turn ON
  }
    if (greenbuttonPresses == 2){
        unsigned long greencurrentMillis = millis();

  if (greencurrentMillis - greenpreviousMillis >= greeninterval) {
    // save the last time you blinked the green LED
    greenpreviousMillis = greencurrentMillis;

    // if the LED is off turn it on and vice-versa:
    if (PORTD == (PORTD|(1<<PORTD5))) {
      PORTD &= ~(1 << PORTD5);
    } else {
      PORTD |= (1 << PORTD5);;
    }
    }
}
if ((greenbuttonPresses == 3)||(greenbuttonPresses == 0)){
      PORTD |= (1 << PORTD5);
    }
    lastyellowPressCount = yellowbuttonPresses;    // track last press count

}
void loop(){
  loopyellow();
  loopgreen();
}
