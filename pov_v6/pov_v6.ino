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
#include "graphics118.h"

#define HALLPIN     8
#define CLOCKPIN2   13
#define CLOCKPIN1   13

#define DATA_A 3
#define DATA_B 9

Adafruit_DotStar strip1 = Adafruit_DotStar(NUM_LEDS / 2, DATA_A, CLOCKPIN1, DOTSTAR_BGR);
Adafruit_DotStar strip2 = Adafruit_DotStar(NUM_LEDS / 2, DATA_B, CLOCKPIN2, DOTSTAR_BGR);

volatile uint32_t rps, // revolution per second
                  revolutions,
                  revolutionDelta, // Time of a single revolution
                  rpsAccumulator, // Accumulates individual revolution time for calculating rps 
                  hallStart; // The time in millis when the hall sensor was last detected
uint16_t *ptr, pixNode, degPos, dpi = 180, dpi2 = 360; 
uint8_t r, g, b, p, x, y, halfLEDS = NUM_LEDS / 2;
double pi = 3.14159;

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
  revolutionDelta = hallStart = millis();
  rps = rpsAccumulator = revolutions = 0; 

  // Enable specialized pin intterupts 
  enableInterruptPin(HALLPIN);

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
  // because that would require inordinate RAM
  if(imageType == PALETTE1)      memcpy_P(palette, imagePalette,  2 * 3);
  else if(imageType == PALETTE4) memcpy_P(palette, imagePalette, 16 * 3);
}

void loop() {
  // Convert angular velocity into degrees at any given point in time
  degPos = ((millis() - hallStart) * 360) / revolutionDelta; 
  
  // dummy calculations to test speed of trinket versus dot-star led strip
  // palette 1 algorithm : try to address 500 LED long strip (2 strips)
  for(int i = 0; i < 100; i++) {
    if(i < halfLEDS) { // For LED strip 1
      x = halfLEDS + 4 * tcos(degPos);
      y = halfLEDS + 4 * tsin(degPos);
    } else { // For LED strip 2
      x = halfLEDS + (4 - halfLEDS) * tcos(degPos + 180);
      y = halfLEDS + (4 - halfLEDS) * tsin(degPos + 180);
    }

    // No idea why these are needed to offset the image into the center
    x-=2;
    y--;

    //palette 1
    pixNode = int(x + y * NUM_LEDS) + 1; 
    ptr = (uint16_t *)&imagePixels[pixNode / 8]; // get the batch of pixels this pixel is in
    p = pgm_read_byte(ptr); 
    p >>= pixNode % 8; // shift over to the bit we want 
    p &= 1; // color index for 0 or 1
  }
    
  for(int i = 0; i < NUM_LEDS; i++) {  
    if(i < halfLEDS) { // For LED strip 1
      x = halfLEDS + i * tcos(degPos);
      y = halfLEDS + i * tsin(degPos);
    } else { // For LED strip 2
      //x = halfLEDS + (i - halfLEDS) * tcos(degPos + 180);
      //y = halfLEDS + (i - halfLEDS) * tsin(degPos + 180);
      x = halfLEDS + (i - halfLEDS) * tcos(degPos + pi);
      y = halfLEDS + (i - halfLEDS) * tsin(degPos + pi);
    }

    // No idea why these are needed to offset the image into the center
    x-=2;
    y--;
 
    switch(imageType) {
      case PALETTE1: {
        pixNode = int(x + y * NUM_LEDS) + 1; 
        ptr = (uint16_t *)&imagePixels[pixNode / 8]; // get the batch of pixels this pixel is in
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
        ptr = (uint16_t *)&imagePixels[(int)pixNode];
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
        ptr = (uint16_t *)&imagePixels[x + y * NUM_LEDS];
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
        ptr = (uint16_t *)&imagePixels[x + y * NUM_LEDS * 3];
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
