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

#define NUMLEDS 118 // Number of LEDs in strip

#define IRPIN       10
#define HALLPIN     8
#define DATAPIN1    3
#define CLOCKPIN1   4
#define DATAPIN2    5
#define CLOCKPIN2   6

Adafruit_DotStar strip1 = Adafruit_DotStar(
  NUMLEDS, DATAPIN1, CLOCKPIN1, DOTSTAR_BRG);

Adafruit_DotStar strip2 = Adafruit_DotStar(
  NUMLEDS, DATAPIN2, CLOCKPIN2, DOTSTAR_BRG);

volatile uint32_t rps, // revolution per second
                  revolutions,
                  revolutionDelta, // Time of a single revolution
                  rpsAccumulator, // Accumulates individual revolution time for calculating rps 
                  hallStart; // The time in millis when the hall sensor was last detected
double radPos, pi = 3.14;

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif
  strip1.begin(); // Initialize pins for output
  strip2.begin();
  strip1.show();  // Turn all LEDs off 
  strip2.show();

  // Init vars
  revolutionDelta = hallStart = millis();
  rps = rpsAccumulator = revolutions = 0; 

  // Enable specialized pin intterupts 
  enableInterruptPin(HALLPIN);
  //enableInterruptPin(IRPIN);

  strip1.setBrightness(10);
  strip2.setBrightness(10);
  
}

void loop() {
  /* Next step: actual math behind displaying an image. 
   *  
   * (millis() - hallStart) = time after we saw the hall sensor last
   * revolutionDelta / (millis() - hallStart) = percent rotation away from hallsensor
   * revoluitonDelta / (millis() - hallStart) * 6.28 = radians away from hall sensor
   * 
   * Convert radian-rotation of a line on an image into a list of pixels to draw. Divide
   * the line into the number of LEDs for one strip. For each LED (division), take its 
   * corresponding point on the line, and find the nearest 4 pixels. Interpolate between 
   * the colors of each pixel as a proportional percentage of the LEDs distance from each
   * pixel, and display that new color with the LED. 
   * 
   * Repeat this calculation with an offset of pi for the other strip.
   */
  
  /* Split the pixel data out onto two LED strips. The conditional hallStart + (revolutionDelta / 2) 
   * is predicting the time it will take to make a half revolution based on the last revolution, 
   * and swapping the content of the strips at that time.
   *
   * old calc was //millis() <= hallStart + (revolutionDelta / 2)
   */
   
  radPos = ((millis() - hallStart) * pi * 2) / revolutionDelta;
  if(radPos < pi) { // half A
    for(int i = 0; i < NUMLEDS; i++) {
      strip1.setPixelColor(i, 0xFF00FF); 
      strip2.setPixelColor(i, 0x00FF00);     
    }
  } else {
    for(int i = 0; i < NUMLEDS; i++) {
      strip1.setPixelColor(i, 0x00FF00); 
      strip2.setPixelColor(i, 0xFF00FF);
    }
  }

  /*if(radPosB > 1.57 && radPosB < 0) {
    for(int i = 0; i < NUMLEDS; i++) {
      strip2.setPixelColor(i, 0x00FF00);     
    }
  } else { // half B
    for(int i = 0; i < NUMLEDS; i++) {
      strip2.setPixelColor(i, 0xFF00FF);
    }
  }*/
  
  /*if(radPosA < 1.57 && radPosA >= 0) { // half A
    for(int i = 0; i < NUMLEDS; i++) {
      strip1.setPixelColor(i, 0xFF00FF); 
      strip2.setPixelColor(i, 0x00FF00);     
    }
  } else { // half B
    for(int i = 0; i < NUMLEDS; i++) {
      strip2.setPixelColor(i, 0xFF00FF);
      strip1.setPixelColor(i, 0x00FF00); 
    }
  }*/

  /* Psudo rps indicator to let me know when the prototype motor is overheating
   * Green is good. Purple is bad -- rps is slowing down or below average.
   */
  if(revolutionDelta >= 160 && revolutionDelta <= 190) {
    strip1.setPixelColor(NUMLEDS - 1, 0xFF0000); // green
    //strip2.setPixelColor(NUMLEDS - 1, 0xFF0000);
  } else {
    strip1.setPixelColor(NUMLEDS - 1, 0x00FFFF); // purple
    //strip2.setPixelColor(NUMLEDS - 1, 0x00FFFF);
  }

  strip1.show();   
  strip2.show();
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


