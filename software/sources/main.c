// tinyProbe - Simple Logic Probe based on ATtiny13A
//
// TinyProbe is a very simple 5V logic probe for TTL and CMOS
// logic. The probe can detect high (HI), low (LO), floating (FL)
// and oscillating (OS) signals and displays them via four LEDs.
//
//                                +-\/-+
// Level Select --- A0 (D5) PB5  1|    |8  Vcc
// Probe ---------- A3 (D3) PB3  2|    |7  PB2 (D2) A1 --- LED (Charlieplex)
// Pull up/down --- A2 (D4) PB4  3|    |6  PB1 (D1) ------ LED (Charlieplex)
//                          GND  4|    |5  PB0 (D0) ------ LED (Charlieplex)
//                                +----+    
//
// RESET pin (PB5) is used as a weak analog input for the 
// TTL/CMOS select switch. You don't need to disable the 
// RESET pin as the voltage won't go below 40% of Vcc.
//
// Controller:  ATtiny13A
// Clockspeed:  1.2 MHz internal
//
// 2020 by Stefan Wagner 
// Project Files (EasyEDA): https://easyeda.com/wagiminator
// Project Files (Github):  https://github.com/wagiminator
// License: http://creativecommons.org/licenses/by-sa/3.0/


// libraries
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// define logic levels for TTL and CMOS at 5V
#define TTL_LOW   164     // ADC value for 0.8V
#define TTL_HIGH  409     // ADC value for 2.0V
#define CMOS_LOW  307     // ADC value for 1.5V
#define CMOS_HIGH 716     // ADC value for 3.5V
#define OSC_DUR    50     // delay time for OS-LED

// global variables
volatile uint8_t isOscillating;

// main function
int main(void) {
  // local variables
  uint16_t valSwitch, valProbe, valHigh, valLow;
  uint8_t  isHigh, isLow, lastHigh, lastLow;
  uint8_t  isFloating;
  
  // setup GPIO, ADC and interrupts
  DDRB    = 0b00000111;   // set LED pins as output, rest as input
  PORTB   = 0b00000000;   // LEDs off, no pullups on input pins
  ADCSRA  = 0b10000011;   // ADC on, prescaler 8
  GIMSK   = 0b00100000;   // turn on pin change interrupts
  SREG   |= 0b10000000;   // enable global interrupts

  // main loop
  while(1) {
    // check logic level selection switch and set high/low thresholds accordingly
    valLow  = TTL_LOW;                // set low value for TTL
    valHigh = TTL_HIGH;               // set high value for TTL
    ADMUX   = 0b00000000;             // set ADC0 against VCC
    ADCSRA |= 0b01000000;             // start ADC conversion
    while(ADCSRA & 0b01000000);       // wait for ADC conversion complete
    valSwitch = ADC;                  // get ADC value
    if (valSwitch < 768) {            // if switch is on CMOS:
      valLow  = CMOS_LOW;             // set low value for CMOS
      valHigh = CMOS_HIGH;            // set high value for CMOS
    }

    // check high frequency oscillation using pin change interrupt on probe line
    DDRB  |= 0b00010000;              // set pull pin to output (PB4)
    PORTB |= 0b00010000;              // pull up probe line (to avoid floating)
    _delay_us(10);                    // wait a bit
    GIFR  |= 0b00100000;              // clear any outstanding pin change interrupt
    PCMSK  = 0b00001000;              // turn on interrupt on probe pin
    _delay_ms(1);                     // wait a millisecond (check >500Hz oscillation)
    PCMSK  = 0b00000000;              // turn off interrupt on probe pin

    // check if probe is floating by testing the behavior when pulling up/down   
    isFloating = PINB;                // get input values (line is already pulled up)
    PORTB &= 0b11101111;              // pull down probe line
    _delay_us(10);                    // wait a bit
    isFloating &= (~PINB);            // get inverse of input values
    isFloating &= 0b00001000;         // mask the probe line (PB3)
    DDRB  &= 0b11101111;              // set pull pin to input (disable pull)
    _delay_us(10);                    // wait a bit
    
    // read voltage on probe line and check if it's logic high or low  
    ADMUX   = 0b00000011;             // set ADC3 against VCC
    ADCSRA |= 0b01000000;             // start ADC conversion
    while(ADCSRA & 0b01000000);       // wait for ADC conversion complete
    valProbe = ADC;                   // get ADC value
    isHigh = (valProbe > valHigh);    // check if probe is logic high
    isLow  = (valProbe < valLow);     // check if probe is logic low

    // check low frequency oscillation
    if (!isFloating && ((isHigh && lastLow) || (isLow && lastHigh))) isOscillating = OSC_DUR;
    lastHigh = isHigh; lastLow  = isLow;
    if (isOscillating) isFloating = 0;    // avoid misdetection
    
    // set the LEDs (charlieplexing)
    PORTB &= 0b11111000;                  // switch off all LEDs
    if (isFloating) PORTB |= 0b00000100;  // if probe is floating: set FL LED
    else {                                // if not floating:
      if (isLow)  PORTB |= 0b00000010;    // if probe is low:      set LO LED
      if (isHigh) PORTB |= 0b00000101;    // if probe is High:     set HI LED
      if (isOscillating) {                // if probe is oscillating:
        PORTB &= 0b11111011;              // set OS LED without interfering
        PORTB |= 0b00000001;              // with HI/LO LED
        isOscillating--;                  // decrease OS LED timer
      }
    }
  }
}

// pin change interrupt service routine
ISR(PCINT0_vect) {
  isOscillating = OSC_DUR;            // oscillating signal on pin change
}
