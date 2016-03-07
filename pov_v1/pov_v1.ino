/* Portions of code taken from Adafruit's double-staff project:
 * https://learn.adafruit.com/pov-dotstar-double-staff?view=all
 *  
 * Written by Zachary Yama for the 2015-16 University of Idaho senior capstone project with 
 * client Dr. Rinker and lead instructors Bruce Bolden and Dan Cordone.
 * 
 * Project Start: 3/6/2016
 * Requirements: Adafruit_DotStar lib: github.com/adafruit/Adafruit_DotStar
 * 
 * Displays a POV image given an array of pixel data from graphics.h 
 * 
 * NOTE: Pin change interrupt code can be used to set any pin as an interrupt. See functions 
 * for more details. Likely useful for wireless communication when its time to add it, as the 
 * trinket pro only has one physical interrupt on pin 3 (INT1). 
 * 
 * "graphics.h" is auto-generated from the python script "convert.py". It takes a set of images
 * in its directory and re-formats their data into pixel and corresponding pallate arrays. They
 * are converted using as little data as possible so that few-colored images can fit on the trinket's
 * 28k of memory. SPI calls are used to access the program-space as if it were flash storage. 
 * 
 */

#include <Arduino.h>
#include <Adafruit_DotStar.h>
#include <SPI.h>       
#include <avr/power.h> 
#include <avr/sleep.h>

typedef uint16_t line_t; 

#include "graphics.h"

#define IRPIN       10
#define HALLPIN     8
#define DATAPIN1    3
#define CLOCKPIN1   4
#define DATAPIN2    5
#define CLOCKPIN2   6

Adafruit_DotStar strip1 = Adafruit_DotStar(
  NUM_LEDS, DATAPIN1, CLOCKPIN1, DOTSTAR_BGR);

Adafruit_DotStar strip2 = Adafruit_DotStar(
  NUM_LEDS, DATAPIN2, CLOCKPIN2, DOTSTAR_BGR);

volatile byte rps, // revolution per second
              revolutions,
              revolutionDelta; // Time of a single revolution
volatile uint32_t rpsAccumulator, // Accumulates individual revolution time for calculating rps 
                  hallStart; // The time in millis when the hall sensor was last detected

uint8_t  imageNumber   = 1,  // Current image being displayed
         imageType,          // Image type: PALETTE[1,4,8] or TRUECOLOR
        *imagePalette,       // -> palette data in PROGMEM
        *imagePixels,        // -> pixel data in PROGMEM
         palette[16][3];     // RAM-based color table for 1- or 4-bit images
line_t   imageLines,         // Number of lines in active image
         imageLine;          // Current line number in image

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif
  strip1.begin(); // Initialize pins for output
  strip2.begin();
  strip1.show();  // Turn all LEDs off 
  strip2.show();

  imageInit();   // Initialize pointers for default image

  // Initialize image stabilizing variables
  revolutionDelta = hallStart = millis();
  rps = rpsAccumulator = revolutions = 0; 

  // Enable specialized pin intterupts 
  enableInterruptPin(HALLPIN);
  //enableInterruptPin(IRPIN);
}

// Initialize global image state for current imageNumber
void imageInit() { 
  imageType    = pgm_read_byte(&images[imageNumber].type);
  imageLines   = pgm_read_word(&images[imageNumber].lines);
  imageLine    = 0;
  imagePalette = (uint8_t *)pgm_read_word(&images[imageNumber].palette);
  imagePixels  = (uint8_t *)pgm_read_word(&images[imageNumber].pixels);
  // 1- and 4-bit images have their color palette loaded into RAM both for
  // faster access and to allow dynamic color changing.  Not done w/8-bit
  // because that would require inordinate RAM (328P could handle it, but
  // I'd rather keep the RAM free for other features in the future).
  if(imageType == PALETTE1)      memcpy_P(palette, imagePalette,  2 * 3);
  else if(imageType == PALETTE4) memcpy_P(palette, imagePalette, 16 * 3);
}

void loop() {
  uint32_t t = millis(); // Current time, milliseconds
  
  switch(imageType) {
    case PALETTE1: { // 1-bit (2 color) palette-based image
      uint8_t  pixelNum = 0, byteNum, bitNum, pixels, idx,
              *ptr = (uint8_t *)&imagePixels[imageLine * NUM_LEDS / 8];
      for(byteNum = NUM_LEDS/8; byteNum--; ) { // Always padded to next byte
        pixels = pgm_read_byte(ptr++);  // 8 pixels of data (pixel 0 = LSB)
        for(bitNum = 8; bitNum--; pixels >>= 1) {
          idx = pixels & 1; // Color table index for pixel (0 or 1)
          strip1.setPixelColor(pixelNum++,
            palette[idx][0], palette[idx][1], palette[idx][2]);
        }
      }
      break;
    }

    case PALETTE4: { // 4-bit (16 color) palette-based image
      uint8_t  pixelNum, p1, p2,
              *ptr = (uint8_t *)&imagePixels[imageLine * NUM_LEDS / 2];
      for(pixelNum = 0; pixelNum < NUM_LEDS; ) {
        p2  = pgm_read_byte(ptr++); // Data for two pixels...
        p1  = p2 >> 4;              // Shift down 4 bits for first pixel
        p2 &= 0x0F;                 // Mask out low 4 bits for second pixel
        strip1.setPixelColor(pixelNum++,
          palette[p1][0], palette[p1][1], palette[p1][2]);
        strip1.setPixelColor(pixelNum++,
          palette[p2][0], palette[p2][1], palette[p2][2]);
      }
      break;
    }

    case PALETTE8: { // 8-bit (256 color) PROGMEM-palette-based image
      uint16_t  o;
      uint8_t   pixelNum,
               *ptr = (uint8_t *)&imagePixels[imageLine * NUM_LEDS];
      for(pixelNum = 0; pixelNum < NUM_LEDS; pixelNum++) {
        o = pgm_read_byte(ptr++) * 3; // Offset into imagePalette
        strip1.setPixelColor(pixelNum,
          pgm_read_byte(&imagePalette[o]),
          pgm_read_byte(&imagePalette[o + 1]),
          pgm_read_byte(&imagePalette[o + 2]));
      }
      break;
    }

    case TRUECOLOR: { // 24-bit ('truecolor') image (no palette)
      uint8_t  pixelNum, r, g, b,
              *ptr = (uint8_t *)&imagePixels[imageLine * NUM_LEDS * 3];
      for(pixelNum = 0; pixelNum < NUM_LEDS; pixelNum++) {
        r = pgm_read_byte(ptr++);
        g = pgm_read_byte(ptr++);
        b = pgm_read_byte(ptr++);
        strip1.setPixelColor(pixelNum, r, g, b);
      }
      break;
    }
  }

  if(++imageLine >= imageLines) imageLine = 0; // Next scanline, wrap around

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
   */
  /*if(millis() <= hallStart + (revolutionDelta / 2)) { // half A
    for(int i = 0; i < NUM_LEDS; i++) {
      strip1.setPixelColor(i, 0xFF00FF); 
      strip2.setPixelColor(i, 0x00FF00);     
    }
  } else { // half B
    for(int i = 0; i < NUM_LEDS; i++) {
      strip2.setPixelColor(i, 0xFF00FF);
      strip1.setPixelColor(i, 0x00FF00); 
    }
  }

  /* Psudo rps indicator to let me know when the prototype motor is overheating
   * Green is good. Purple is bad -- rps is slowing down or below average.
   */
  /*if(revolutionDelta >= 160 && revolutionDelta <= 190) {
    strip1.setPixelColor(NUM_LEDS - 1, 0xFF0000); // green
    strip2.setPixelColor(NUM_LEDS - 1, 0xFF0000);
  } else {
    strip1.setPixelColor(NUM_LEDS - 1, 0x00FFFF); // purple
    strip2.setPixelColor(NUM_LEDS - 1, 0x00FFFF);
  }*/

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


