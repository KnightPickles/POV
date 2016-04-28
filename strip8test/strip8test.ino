/* Written by Zachary Yama for the 2015-16 University of Idaho senior capstone project with 
 * client Dr. Rinker and lead instructors Bruce Bolden and Dan Cordone.
 * 
 * Project Start: 3/4/2016
 * 
 * POV prototype code. Displays a simple ring half-blue and half-red to test our concept of timing 
 * that will be used to reset images into a still-standing state on the final product.
 * 
 * The outer LED is purple if the rps are indicative of the prototype's fan-motor overheating. 
 * Otherwise, the outer LED is green. 
 * 
 * NOTE: Pin change interrupt code can be used to set pin as an interrupt. See functions for
 * more details. Likely useful for wireless communication when its time to add it, as the 
 * trinket pro only has one physical interrupt on pin 3. 
 * 
 * It's also worth mentioning that there's a visibly noticable difference in the refresh rate
 * of processing 120 LEDs versus 14 on the prototype. Particularly when using halldemo.ino, 
 * which displays two colored half-circles, the dividing line between the colors is much larger 
 * with 120, versus 14. 
 * 
 */


#include <Adafruit_DotStar.h>
// Because conditional #includes don't work w/Arduino sketches...
#include <SPI.h>         // COMMENT OUT THIS LINE FOR GEMMA OR TRINKET
#include <avr/power.h> // ENABLE THIS LINE FOR GEMMA OR TRINKET
#include <avr/sleep.h>

#define NUM_LEDS 56 // Number of LEDs in strip

#define NUM_STRIPS  8
#define NUM_ST_LEDS 14

#define HALLPIN 8

#define DATA_A1 3
#define DATA_A2 4
#define DATA_A3 5
#define DATA_A4 6

#define DATA_B1 9
#define DATA_B2 10
#define DATA_B3 11
#define DATA_B4 12

#define CLOCK_A 13
#define CLOCK_B 13

Adafruit_DotStar LED_A1 = Adafruit_DotStar(NUM_ST_LEDS, DATA_A1, CLOCK_A, DOTSTAR_BGR);
Adafruit_DotStar LED_A2 = Adafruit_DotStar(NUM_ST_LEDS, DATA_A2, CLOCK_A, DOTSTAR_BGR);
Adafruit_DotStar LED_A3 = Adafruit_DotStar(NUM_ST_LEDS, DATA_A3, CLOCK_A, DOTSTAR_BGR);
Adafruit_DotStar LED_A4 = Adafruit_DotStar(NUM_ST_LEDS, DATA_A4, CLOCK_A, DOTSTAR_BGR);

Adafruit_DotStar LED_B1 = Adafruit_DotStar(NUM_ST_LEDS, DATA_B1, CLOCK_B, DOTSTAR_BGR);
Adafruit_DotStar LED_B2 = Adafruit_DotStar(NUM_ST_LEDS, DATA_B2, CLOCK_B, DOTSTAR_BGR);
Adafruit_DotStar LED_B3 = Adafruit_DotStar(NUM_ST_LEDS, DATA_B3, CLOCK_B, DOTSTAR_BGR);
Adafruit_DotStar LED_B4 = Adafruit_DotStar(NUM_ST_LEDS, DATA_B4, CLOCK_B, DOTSTAR_BGR);

volatile uint32_t rps, // revolution per second
                  revolutions,
                  revolutionDelta, // Time of a single revolution
                  rpsAccumulator, // Accumulates individual revolution time for calculating rps 
                  hallStart; // The time in millis when the hall sensor was last detected
int radPos;

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif

  // Init vars
  revolutionDelta = hallStart = millis();
  rps = rpsAccumulator = revolutions = 0; 

  LED_A1.begin();
  LED_A2.begin();
  LED_A3.begin();
  LED_A4.begin();
  LED_B1.begin();
  LED_B2.begin();
  LED_B3.begin();
  LED_B4.begin();

  LED_A1.clear();
  LED_A2.clear();
  LED_A3.clear();
  LED_A4.clear();
  LED_B1.clear();
  LED_B2.clear();
  LED_B3.clear();
  LED_B4.clear();

  LED_A1.show();
  LED_A2.show();
  LED_A3.show();
  LED_A4.show();
  LED_B1.show();
  LED_B2.show();
  LED_B3.show();
  LED_B4.show();

  enableInterruptPin(HALLPIN);
}

void loop() {
  for(int pixelNum = 0; pixelNum < NUM_LEDS; pixelNum++) {
    if(pixelNum < NUM_ST_LEDS) {
      LED_A1.setPixelColor(pixelNum, 0xff0000);
      LED_B1.setPixelColor(pixelNum, 0xff0000);
    } else if(pixelNum >= NUM_ST_LEDS && pixelNum < NUM_ST_LEDS * 2) {
      LED_A2.setPixelColor(pixelNum - NUM_ST_LEDS, 0x00ff00);
      LED_B2.setPixelColor(pixelNum - NUM_ST_LEDS, 0x00ff00);
    } else if(pixelNum >= NUM_ST_LEDS * 2 && pixelNum < NUM_ST_LEDS * 3) {
      LED_A3.setPixelColor(pixelNum - NUM_ST_LEDS * 2, 0x0000ff);
      LED_B3.setPixelColor(pixelNum - NUM_ST_LEDS * 2, 0x0000ff);
    } else if(pixelNum >= NUM_ST_LEDS * 3 && pixelNum < NUM_ST_LEDS * 4) {
      LED_A4.setPixelColor(pixelNum - NUM_ST_LEDS * 3, 0xff00ff);
      LED_B4.setPixelColor(pixelNum - NUM_ST_LEDS * 3, 0xff00ff);
    }
  }

  LED_A1.show();
  LED_A2.show();
  LED_A3.show();
  LED_A4.show();
  LED_B1.show();
  LED_B2.show();
  LED_B3.show();
  LED_B4.show();
}


// ------ Intterupt Functionality ----- //

// Add any interrupt pin by swapping around pin change interrupts internally.
void enableInterruptPin(byte pin) {
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

// Puts microcontroller to sleep. Wake on interrupt. 
void sleep() {
  // Sleep states
  // SLEEP_MODE_IDLE
  // SLEEP_MODE_ADC
  // SLEEP_MODE_PWR_SAVE
  // SLEEP_MODE_STANDBY
  // SLEEP_MODE_PWR_DOWN 
  
  set_sleep_mode(SLEEP_MODE_IDLE); // Set sleep mode.
  sleep_enable(); // Enable sleep mode.
  sleep_mode(); // Enter sleep mode.
  // Point of interrupt waking and resuming instruction execution
  sleep_disable(); // Disable sleep mode after waking.
}

// handle pin change interrupt for D0 to D7 
ISR (PCINT2_vect) { }

// handle pin change interrupt for D8 to D13
ISR (PCINT0_vect) { 
  if(digitalRead(HALLPIN) == LOW) { // hall sensor interrupt; update timings
    revolutions++;
    revolutionDelta = millis() - hallStart; 
    hallStart = millis();
    rpsAccumulator += revolutionDelta; 
    if(rpsAccumulator >= 1000) { // Dealing in milliseconds
      rps = rpsAccumulator / 1000; 
      rpsAccumulator = 0;
    }
  }
} 

// handle pin change interrupt for A0 to A5
ISR (PCINT1_vect) { }


