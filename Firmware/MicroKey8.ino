#include <Arduino.h>
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
  // In this case, CLKPS3..0 need to be set to 0000 for a division factor of 1
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

  // Print message to the serial monitor (/dev/ttyACM*)
  Serial.begin(9600);
  delay(1000);
  Serial.println("initialized");

}

void loop() {

  Serial.println("running");
  Serial.print("CLKPR: ");
  Serial.println(CLKPR, BIN);
  delay(500);
  
}
