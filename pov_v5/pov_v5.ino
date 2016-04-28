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
#include <avr/power.h>
#include <avr/sleep.h>
#include <SPI.h>

typedef uint16_t line_t;

// CONFIGURABLE STUFF ------------------------------------------------------

#include "smileD.h" // Graphics data is contained in this header file.
// It's generated using the 'convert.py' Python script.  Various image
// formats are supported, trading off color fidelity for PROGMEM space.
// Handles 1-, 4- and 8-bit-per-pixel palette-based images, plus 24-bit
// truecolor.  1- and 4-bit palettes can be altered in RAM while running
// to provide additional colors, but be mindful of peak & average current
// draw if you do that!  Power limiting is normally done in convert.py
// (keeps this code relatively small & fast).

// Ideally you use hardware SPI as it's much faster, though limited to
// specific pins.  If you really need to bitbang DotStar data & clock on
// different pins, optionally define those here:

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

void imageInit(void);

void setup() {
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

  imageInit();   // Initialize pointers for default image
  enableInterruptPin(HALLPIN);
}

// GLOBAL STATE STUFF ------------------------------------------------------
volatile uint32_t rps, // revolution per second
                  revolutions,
                  revolutionDelta, // Time of a single revolution
                  aveRevolution,
                  rpsAccumulator, // Accumulates individual revolution time for calculating rps 
                  hallStart; // The time in millis when the hall sensor was last detected

uint16_t degPos, pideg = 180, pi2deg = 360;

uint8_t  imageNumber   = 0,  // Current image being displayed
         imageType,          // Image type: PALETTE[1,4,8] or TRUECOLOR
        *imagePalette,       // -> palette data in PROGMEM
        *imagePixels,        // -> pixel data in PROGMEM
         palette[16][3];     // RAM-based color table for 1- or 4-bit images
line_t   imageLines,         // Number of lines in active image
         imageLine1,          // Current line number in image
         imageLine2;

void imageInit() { // Initialize global image state for current imageNumber
  imageType    = pgm_read_byte(&images[imageNumber].type);
  imageLines   = pgm_read_word(&images[imageNumber].lines);
  imageLine1   = 0;
  imageLine2   = 0;
  imagePalette = (uint8_t *)pgm_read_word(&images[imageNumber].palette);
  imagePixels  = (uint8_t *)pgm_read_word(&images[imageNumber].pixels);
  // 1- and 4-bit images have their color palette loaded into RAM both for
  // faster access and to allow dynamic color changing. 
  if(imageType == PALETTE1)      memcpy_P(palette, imagePalette,  2 * 3);
  else if(imageType == PALETTE4) memcpy_P(palette, imagePalette, 16 * 3);

  LED_A1.setBrightness(255);
  LED_A2.setBrightness(255);
  LED_A3.setBrightness(255);
  LED_A4.setBrightness(255);
  LED_B1.setBrightness(255);
  LED_B2.setBrightness(255);
  LED_B3.setBrightness(255);
  LED_B4.setBrightness(255);
}

void nextImage(void) {
  if(++imageNumber >= NUM_IMAGES) imageNumber = 0;
  imageInit();
}

void prevImage(void) {
  imageNumber = imageNumber ? imageNumber - 1 : NUM_IMAGES - 1;
  imageInit();
}

void loop() {
  //degPos = ((millis() - hallStart) * pi2deg) / revolutionDelta; 
  //imageLine1 = (imageLines * degPos) / pi2deg;
  //imageLine2 = (imageLines * (degPos + pideg)) / pi2deg;
  //if(imageLine2 > imageLines) imageLine2 -= imageLines; // wrap

  imageLine1 = ((millis() - hallStart) * imageLines) / revolutionDelta; 
  imageLine2 = imageLine1;
  imageLine1 += imageLines/2;
  if(imageLine1 >= imageLines) imageLine1 -= imageLines;
  if(imageLine2 >= imageLines) imageLine2 -= imageLines;

 

  /* Dummy calculations that test the speed of a trinket versus a dot-star led strip
   * The math behind dertermining a pixels corresponding LED is not a hindering factor. 
   * 
   * It's worth noting that the performance is slightly better on an 8 piece strip than
   * on a 2 piece strip. 
   * 
   * There are clearly multiple limiting factors in code and hardware. The bus may be too 
   * slow to pump all that data out, or the strip's timing for the clock we run at may be 
   * too slow. The image we upload can only be so big, so 360 degrees of accuracy may not
   * be enough. 
   */
  /*uint8_t  r, g, b;
  uint8_t  pixelNum, p1A, p2A, p1B, p2B,
    *ptrA = (uint8_t *)&imagePixels[imageLine1 * NUM_LEDS / 2],
    *ptrB = (uint8_t *)&imagePixels[imageLine2 * NUM_LEDS / 2];
  for(int pixelNum = 0; pixelNum < 1000; pixelNum+=2) {
    p2A  = pgm_read_byte(ptrA++); // Data for two pixels...
    p1A  = p2A >> 4;              // Shift down 4 bits for first pixel
    p2A &= 0x0F;                 // Mask out low 4 bits for second pixel
    p2B  = pgm_read_byte(ptrB++);
    p1B  = p2B >> 4;
    p2B &= 0x0F; 
    // Pretend assign data on 8 strip sections
    for(int i = 0; i < 8; i++) {
      r = palette[p1A][0];
      b = palette[p2A][1];
      g = palette[p1B][2];
    }
  }*/
 
  
  // Transfer one scanline from pixel data to LED strip:
  switch(imageType) {

    case PALETTE1: { // 1-bit (2 color) palette-based image
      // Size of address not large enough to utilize images with lines > 360
      uint8_t  pixelNum = 0, byteNum, bitNum, pixelsA, pixelsB, idA, idB;
      uint8_t *ptrA = (uint8_t *)&imagePixels[imageLine1 * NUM_LEDS / 8],
               *ptrB = (uint8_t *)&imagePixels[imageLine2 * NUM_LEDS / 8];
      for(byteNum = NUM_LEDS/8; byteNum--;) { // Always padded to next byte
        pixelsA = pgm_read_byte(ptrA++);  // 8 pixels of data (pixel 0 = LSB)
        pixelsB = pgm_read_byte(ptrB++);
        for(bitNum = 8; bitNum--; pixelsA >>= 1, pixelsB >>= 1, pixelNum++) {
          idA = pixelsA & 1; // Color table index for pixel (0 or 1)
          idB = pixelsB & 1;
          if(pixelNum < NUM_ST_LEDS) {
            LED_A1.setPixelColor(pixelNum, palette[idA][0], palette[idA][1], palette[idA][2]);
            LED_B1.setPixelColor(pixelNum, palette[idB][0], palette[idB][1], palette[idB][2]);
          } else if(pixelNum >= NUM_ST_LEDS && pixelNum < NUM_ST_LEDS * 2) {
            LED_A2.setPixelColor(pixelNum - NUM_ST_LEDS, palette[idA][0], palette[idA][1], palette[idA][2]);
            LED_B2.setPixelColor(pixelNum - NUM_ST_LEDS, palette[idB][0], palette[idB][1], palette[idB][2]);
          } else if(pixelNum >= NUM_ST_LEDS * 2 && pixelNum < NUM_ST_LEDS * 3) {
            LED_A3.setPixelColor(pixelNum - NUM_ST_LEDS * 2, palette[idA][0], palette[idA][1], palette[idA][2]);
            LED_B3.setPixelColor(pixelNum - NUM_ST_LEDS * 2, palette[idB][0], palette[idB][1], palette[idB][2]);
          } else if(pixelNum >= NUM_ST_LEDS * 3 && pixelNum < NUM_ST_LEDS * 4) {
            LED_A4.setPixelColor(pixelNum - NUM_ST_LEDS * 3, palette[idA][0], palette[idA][1], palette[idA][2]);
            LED_B4.setPixelColor(pixelNum - NUM_ST_LEDS * 3, palette[idB][0], palette[idB][1], palette[idB][2]);
          }
        }
      }
      break;
    }

    case PALETTE4: { // 4-bit (16 color) palette-based image
      uint8_t  pixelNum, p1A, p2A, p1B, p2B;
      uint8_t *ptrA = (uint8_t *)&imagePixels[imageLine1 * NUM_LEDS / 2],
              *ptrB = (uint8_t *)&imagePixels[imageLine2 * NUM_LEDS / 2];
      for(pixelNum = 0; pixelNum < NUM_LEDS; pixelNum+=2) {
        p2A  = pgm_read_byte(ptrA++); // Data for two pixels...
        p1A  = p2A >> 4;              // Shift down 4 bits for first pixel
        p2A &= 0x0F;                 // Mask out low 4 bits for second pixel
        p2B  = pgm_read_byte(ptrB++);
        p1B  = p2B >> 4;
        p2B &= 0x0F;                 
        if(pixelNum < NUM_ST_LEDS) {
          LED_A1.setPixelColor(pixelNum, palette[p1A][0], palette[p1A][1], palette[p1A][2]);
          LED_A1.setPixelColor(pixelNum + 1, palette[p2A][0], palette[p2A][1], palette[p2A][2]);
          LED_B1.setPixelColor(pixelNum, palette[p1B][0], palette[p1B][1], palette[p1B][2]);
          LED_B1.setPixelColor(pixelNum + 1, palette[p2B][0], palette[p2B][1], palette[p2B][2]);
        } else if(pixelNum >= NUM_ST_LEDS && pixelNum < NUM_ST_LEDS * 2) {
          LED_A2.setPixelColor(pixelNum - NUM_ST_LEDS, palette[p1A][0], palette[p1A][1], palette[p1A][2]);
          LED_A2.setPixelColor(pixelNum + 1 - NUM_ST_LEDS, palette[p2A][0], palette[p2A][1], palette[p2A][2]);
          LED_B2.setPixelColor(pixelNum - NUM_ST_LEDS, palette[p1B][0], palette[p1B][1], palette[p1B][2]);
          LED_B2.setPixelColor(pixelNum + 1 - NUM_ST_LEDS, palette[p2B][0], palette[p2B][1], palette[p2B][2]);
        } else if(pixelNum >= NUM_ST_LEDS * 2 && pixelNum < NUM_ST_LEDS * 3) {
          LED_A3.setPixelColor(pixelNum - NUM_ST_LEDS * 2, palette[p1A][0], palette[p1A][1], palette[p1A][2]);
          LED_A3.setPixelColor(pixelNum + 1  - NUM_ST_LEDS * 2, palette[p2A][0], palette[p2A][1], palette[p2A][2]);
          LED_B3.setPixelColor(pixelNum  - NUM_ST_LEDS * 2, palette[p1B][0], palette[p1B][1], palette[p1B][2]);
          LED_B3.setPixelColor(pixelNum + 1  - NUM_ST_LEDS * 2, palette[p2B][0], palette[p2B][1], palette[p2B][2]);
        } else if(pixelNum >= NUM_ST_LEDS * 3 && pixelNum < NUM_ST_LEDS * 4) {
          LED_A4.setPixelColor(pixelNum - NUM_ST_LEDS * 3, palette[p1A][0], palette[p1A][1], palette[p1A][2]);
          LED_A4.setPixelColor(pixelNum + 1 - NUM_ST_LEDS * 3, palette[p2A][0], palette[p2A][1], palette[p2A][2]);
          LED_B4.setPixelColor(pixelNum - NUM_ST_LEDS * 3, palette[p1B][0], palette[p1B][1], palette[p1B][2]);
          LED_B4.setPixelColor(pixelNum + 1 - NUM_ST_LEDS * 3, palette[p2B][0], palette[p2B][1], palette[p2B][2]);
        }
      }
      break;
    }

    case PALETTE8: { // 8-bit (256 color) PROGMEM-palette-based image
      uint16_t  o;
      uint8_t   pixelNum,
               *ptr = (uint8_t *)&imagePixels[imageLine1 * NUM_LEDS];
      for(pixelNum = 0; pixelNum < NUM_LEDS; pixelNum++) {
        o = pgm_read_byte(ptr++) * 3; // Offset into imagePalette
        /*strip1.setPixelColor(pixelNum,
          pgm_read_byte(&imagePalette[o]),
          pgm_read_byte(&imagePalette[o + 1]),
          pgm_read_byte(&imagePalette[o + 2]));*/
      }
      
      ptr = (uint8_t *)&imagePixels[imageLine2 * NUM_LEDS];
      for(pixelNum = 0; pixelNum < NUM_LEDS; pixelNum++) {
        o = pgm_read_byte(ptr++) * 3; // Offset into imagePalette
        /*strip2.setPixelColor(pixelNum,
          pgm_read_byte(&imagePalette[o]),
          pgm_read_byte(&imagePalette[o + 1]),
          pgm_read_byte(&imagePalette[o + 2]));*/
      }
      break;
    }

    case TRUECOLOR: { // 24-bit ('truecolor') image (no palette)
      uint8_t  pixelNum, r, g, b,
              *ptr = (uint8_t *)&imagePixels[imageLine1 * NUM_LEDS * 3];
      for(pixelNum = 0; pixelNum < NUM_LEDS; pixelNum++) {
        r = pgm_read_byte(ptr++);
        g = pgm_read_byte(ptr++);
        b = pgm_read_byte(ptr++);
        //strip1.setPixelColor(pixelNum, r, g, b);
      }

      ptr = (uint8_t *)&imagePixels[imageLine2 * NUM_LEDS * 3];
      for(pixelNum = 0; pixelNum < NUM_LEDS; pixelNum++) {
        r = pgm_read_byte(ptr++);
        g = pgm_read_byte(ptr++);
        b = pgm_read_byte(ptr++);
        //strip2.setPixelColor(pixelNum, r, g, b);
      }
      break;
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

// handle pin change interrupt for D0 to D7 
ISR (PCINT2_vect) { }

// handle pin change interrupt for D8 to D13
ISR (PCINT0_vect) { 
  if(digitalRead(HALLPIN) == LOW) { // hall sensor interrupt; update timings
    revolutions++;
    revolutionDelta = millis() - hallStart; 
    hallStart = millis();
    rpsAccumulator += revolutionDelta; 
    aveRevolution = rpsAccumulator / revolutions;
    /*if(rpsAccumulator >= 1000) { // Dealing in milliseconds
      rps = rpsAccumulator / 1000; 
      rpsAccumulator = 0;
    }*/
  }
} 

// handle pin change interrupt for A0 to A5
ISR (PCINT1_vect) { }


