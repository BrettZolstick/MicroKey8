#include <Arduino.h>
#include <Keyboard.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

void setup() {

  // Running into a bug where the MCU's watchdog keeps thinking the program is stuck.
  // It keeps forcing a reset every time it boots. Disabling the watchdog timer fixes this.
  // 
  // Clear the watchdog reset flag and then disable the watchdog timer.
  MCUSR &= ~(1 << WDRF);
  wdt_disable();

  // The ATMEGA32U4 has a System Clock Prescaler, which divides the clock by a set amount.
  // By default, it divides by 8. Its 16Mhz signal from the crystal goes down to 2Mhz.
  // This can be overwritten with a special proceedure. (see 6.8.1 and 6.11.4 in the ATMEGA32U4's datasheet)
  //
  //   CLKPR is the Clock Prescaler Register
  //   7 (CLKPCE)  | 6 -         | 5 -         | 4 -         | 3 (CLKPS3)  | 2 (CLKPS2)  | 1 (CLKPS1)  | 0 (CLKPS0)
  //   R/W           R             R             R             R/W           R/W           R/W           R/W
  //   0             0             0             0             0             0             1             1
  // 
  //   See Table 6-10  for available division factors(6.11.4 in datasheet)
  //
  // In this case, CLKPS3..0 need to be set to 0000 for a division factor of 1 so it runs at it's full 16Mhz
  // 
  //
  // Temporarily disable interrupts (clear interrupt flag)
  cli();
  // Set Clock Prescaler Change Enable (CLKPCE) to 1 and all other CLKPR bits to 0
  CLKPR = (1 << CLKPCE);
  // Within 4 clock cycles, write the desired value to CLKPS3..0 while writing a 0 to CLKPCE
  CLKPR = 0; // 00000000 = divide by 1
  // Re-enable interrupts (set interrupt flag)
  sei();

  // Configure all the GPIO pins that will be needed
  DDRB = 0b00000000; // set registers B and D as inputs
  DDRD = 0b00000000;
  PORTB = 0b11111111; // enable the internal pull-up resistors on registers B and D 
  PORTD = 0b11111111;

  // Enable the serial monitor and send a message (/dev/ttyACM*)
  Serial.begin(9600);
  delay(1000);
  Serial.println("initialized");
}


unsigned long debounceMicros = 2000;
char previousKey;

struct Switch {
  byte pin;

  char key;

  bool topState;
  bool bottomState;

  bool switchState;
  bool previousSwitchState;

  unsigned long switchStateChangeTimestamp;
};
void readPins(Switch &sw);
void getSwitchState(Switch &sw);
void updateSwitchState(Switch &sw);

void readPins(Switch &sw) {
    uint8_t mask = (1 << sw.pin); // create a mask for only the pin that you want to read
    sw.topState = ((PIND & mask) != 0); // And the two 8 bit values, if its not 00000000, then the masked pin must be 1
    sw.bottomState = ((PINB & mask) != 0); // 0 = contact with ground, 1 = no contact
}

void getSwitchState(Switch &sw){
  // if the switch is currently unpressed, change it to be pressed as soon as the top contact breaks
  if (!sw.switchState) {
    if (sw.topState){
      sw.switchState = true;
    }
  }
  // if the switch is currently pressed, change it to be unpressed as soon as the bottom contact breaks
  if (sw.switchState) {
    if (sw.bottomState){
      sw.switchState = false;
    }
  }
}

void updateSwitchState(Switch &sw){
  // exit if no change
  if (sw.switchState == sw.previousSwitchState){
    return;
  }

  // exit if during the debounce timeframe
  if ((micros() - sw.switchStateChangeTimestamp) < debounceMicros ){
    return;
  }

  // otherwise, send key press
  sw.switchStateChangeTimestamp = micros();
  if (sw.switchState){
    // don't send a press if not in full alt order
    if (sw.key == 'a' && (previousKey == 'a' || previousKey == 's')){return;}
    if (sw.key == 's' && (previousKey == 's' || previousKey == 'd')){return;}
    if (sw.key == 'd' && (previousKey == 'd' || previousKey == 'a')){return;}

    // otherwise, send the key press
    Keyboard.press(sw.key);
    previousKey = sw.key;
  }else{
    Keyboard.release(sw.key);
  }

  // set previous switch state for the next loop
  sw.previousSwitchState = sw.switchState;
}

Switch switches[] = {
  {0,'a'},
  {1,'s'},
  {2,'d'},
  {3,'v'},
  {4,';'},
  {5,'l'},
  {6,'k'},
  {7,'n'},
};

void loop() {

  for (int i = 0; i < 8; i++){
    readPins(switches[i]);
    getSwitchState(switches[i]);
    updateSwitchState(switches[i]);
  }
}
