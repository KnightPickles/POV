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
 * are converted into as few bytes as possible so that few-colored images can fit on the trinket's
 * 28k memory. SPI calls are used to access the program-space as if it were flash storage. 
 * 
 * It's also worth mentioning that there's a visibly noticable difference in the refresh rate
 * of processing 120 LEDs versus 14 on the prototype. Particularly when using halldemo.ino, 
 * which displays two colored half-circles, the dividing line between the colors is much larger 
 * with 120, versus 14. 
 * 
 */

#include <Arduino.h>
#include <Adafruit_DotStar.h>
#include <SPI.h>       
#include <avr/power.h> 
#include <avr/sleep.h>

typedef uint16_t line_t; 

#include "trigtable.h"
#include "let.h"

#define HALLPIN     8
#define DATAPIN1    3
#define CLOCKPIN1   4
#define DATAPIN2    5
#define CLOCKPIN2   6

#define D_A2        10
#define D_A3        11
#define D_B2        12
#define D_B3        13

Adafruit_DotStar strip1 = Adafruit_DotStar(NUM_LEDS, DATAPIN1, CLOCKPIN1, DOTSTAR_BGR);
Adafruit_DotStar strip2 = Adafruit_DotStar(NUM_LEDS, DATAPIN2, CLOCKPIN2, DOTSTAR_BGR);
Adafruit_DotStar stripA2 = Adafruit_DotStar(NUM_LEDS, D_A2, CLOCKPIN2, DOTSTAR_BGR);
Adafruit_DotStar stripA3 = Adafruit_DotStar(NUM_LEDS, D_A3, CLOCKPIN2, DOTSTAR_BGR);
Adafruit_DotStar stripB2 = Adafruit_DotStar(NUM_LEDS, D_B2, CLOCKPIN2, DOTSTAR_BGR);
Adafruit_DotStar stripB3 = Adafruit_DotStar(NUM_LEDS, D_B3, CLOCKPIN2, DOTSTAR_BGR);

volatile uint32_t rps, // revolution per second
                  revolutions,
                  revolutionDelta, // Time of a single revolution
                  rpsAccumulator, // Accumulates individual revolution time for calculating rps 
                  hallStart; // The time in millis when the hall sensor was last detected
double px, py, percent, radPos, degPos, pi = 3.14159, pi2 = 2 * pi, dpi = 180, dpi2 = 360; 
uint32_t *ptr, pixNode;
uint8_t r, g, b, p, x, y;
int halfLEDS = NUM_LEDS / 2;
int sixthLEDS = NUM_LEDS / 6;

uint8_t  imageNumber   = 0,  // Current image being displayed
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
  revolutionDelta = hallStart = micros();
  rps = rpsAccumulator = revolutions = 0; 

  // Enable specialized pin intterupts 
  enableInterruptPin(HALLPIN);
  //enableInterruptPin(IRPIN);

  strip1.setBrightness(50);
  strip2.setBrightness(50);
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
  // Convert angular velocity into degrees at any given point in time
  degPos = ((micros() - hallStart) * 360) / revolutionDelta; 
    
  for(int i = 0; i < NUM_LEDS; i++) { 
    stripA2.setPixelColor(i, 0,0,0);
    stripA3.setPixelColor(i, 0,0,0);
    stripB2.setPixelColor(i, 0,0,0);
    stripB3.setPixelColor(i, 0,0,0);
    
    if(i < halfLEDS) { // For LED strip A
      x = halfLEDS + i * icos(degPos);
      y = halfLEDS + i * isin(degPos);
    } else { // For LED strip B
      x = halfLEDS + (i - halfLEDS) * icos(degPos + 180);
      y = halfLEDS + (i - halfLEDS) * isin(degPos + 180);
    }

    // No idea why these are needed to offset the image into the center
    x-=2;
    y--;
 
    switch(imageType) {
      case PALETTE1: {
        pixNode = int(x + y * NUM_LEDS) + 1; 
        ptr = (uint32_t *)&imagePixels[pixNode / 8]; // get the batch of pixels this pixel is in
        p = pgm_read_byte(ptr);
        p >>= pixNode % 8; // shift over to the bit we want 
        p &= 1; // color index for 0 or 1
        if(i < halfLEDS) 
          strip1.setPixelColor(i, palette[p][0], palette[p][1], palette[p][2]);
        else strip2.setPixelColor(i - halfLEDS, palette[p][0], palette[p][1], palette[p][2]);
        break;
      }

      case PALETTE4: {
        pixNode = (x + y * NUM_LEDS) / 2; // Each byte contains information for 2 pixels
        ptr = (uint32_t *)&imagePixels[(int)pixNode];
        p = pgm_read_byte(ptr); // Data for two pixels... [ex 0x21]
        if(pixNode == (int)pixNode) // if whole number -> pixel #1, else pixel #2
          p >>= 4; // Shift down 4 bits for first pixel [2 in 0x21]
        else p &= 0x0F;  // Mask out low 4 bits for second pixel [1 in 0x21]
        if(i < halfLEDS) 
          strip1.setPixelColor(i, palette[p][0], palette[p][1], palette[p][2]);
        else strip2.setPixelColor(i - halfLEDS, palette[p][0], palette[p][1], palette[p][2]);
        break;  
      }

      case PALETTE8: {
        ptr = (uint32_t *)&imagePixels[x + y * NUM_LEDS];
        p = pgm_read_byte(ptr) * 3; // offset into the image palette
        r = pgm_read_byte(&imagePalette[p]);
        g = pgm_read_byte(&imagePalette[p + 1]);
        b = pgm_read_byte(&imagePalette[p + 2]);
        if(i < halfLEDS) 
          strip1.setPixelColor(i, r, g, b);
        else strip2.setPixelColor(i - halfLEDS, r, g, b);
        break;
      }
      
      case TRUECOLOR: {
        ptr = (uint32_t *)&imagePixels[x + y * NUM_LEDS * 3];
        r = pgm_read_byte(ptr++);
        g = pgm_read_byte(ptr++);
        b = pgm_read_byte(ptr++);
        if(i < halfLEDS) 
          strip1.setPixelColor(i, r, g, b);
        else strip2.setPixelColor(i - halfLEDS, r, g, b);
        break;  
      }
    }
  }

  strip1.show();   
  strip2.show();
  stripA2.show();
  stripA3.show();
  stripB2.show();
  stripB3.show();
}

// ------ Intterupt Functionality ----- //

// Add any interrupt pin by swapping around pin change interrupts internally.
void enableInterruptPin(byte pin) {
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

// handle pin change interrupt for D0 to D7 
ISR (PCINT2_vect) { }

// handle pin change interrupt for D8 to D13
ISR (PCINT0_vect) { 
  if(digitalRead(HALLPIN) == LOW) { // hall sensor interrupt; update timings
    revolutions++;
    revolutionDelta = micros() - hallStart; 
    hallStart = micros();
    rpsAccumulator += revolutionDelta; 
    if(rpsAccumulator >= 1000) { // Dealing in milliseconds
      rps = rpsAccumulator / 1000; 
      rpsAccumulator = 0;
    }
  }
} 

// handle pin change interrupt for A0 to A5
ISR (PCINT1_vect) { }


