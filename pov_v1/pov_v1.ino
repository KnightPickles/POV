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

#include "graphics118.h"

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

volatile uint32_t rps, // revolution per second
                  revolutions,
                  revolutionDelta, // Time of a single revolution
                  rpsAccumulator, // Accumulates individual revolution time for calculating rps 
                  hallStart; // The time in millis when the hall sensor was last detected
double x, y, px, py, percent, radPos, pi = 3.14159;
uint32_t *ptr, pixNode;
uint8_t r, g, b, p;

uint8_t  imageNumber   = 0,  // Current image being displayed
         imageType,          // Image type: PALETTE[1,4,8] or TRUECOLOR
        *imagePalette,       // -> palette data in PROGMEM
        *imagePixels,        // -> pixel data in PROGMEM
         palette[16][3];     // RAM-based color table for 1- or 4-bit images
line_t   imageLines,         // Number of lines in active image
         imageLine;          // Current line number in image

uint32_t lastLoopTime = millis();
int TARGET_FPS = 15;
uint32_t OPTIMAL_TIME = 1000 / TARGET_FPS;
uint32_t lastFpsTime = 0;


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

  strip1.setBrightness(10);
  strip2.setBrightness(10);
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
  //radPos = ((millis() - hallStart) * pi * 2) / revolutionDelta;

  /*uint32_t now = millis();
  uint32_t upL = now - lastLoopTime;
  lastLoopTime = now;
  double delta = upL / ((double)OPTIMAL_TIME); 

  lastFpsTime += upL;
  if(lastFpsTime >= 1000)
    lastFpsTime = 0;*/

  radPos = ((millis() - hallStart) * pi * 2) / revolutionDelta;  
  for(int i = 0; i < NUM_LEDS; i++) {

    // Radial position to quadratic.
    if(i < NUM_LEDS / 2) {
      x = (NUM_LEDS / 2) + i * cos(radPos);
      y = (NUM_LEDS / 2) + i * sin(radPos);
    } else {
      x = (NUM_LEDS / 2) + (i % (NUM_LEDS / 2)) * cos(radPos + pi);
      y = (NUM_LEDS / 2) + (i % (NUM_LEDS / 2)) * sin(radPos + pi);
    }
    //leds[i].x = (NUM_LEDS / 2) + (i > NUM_LEDS / 2 ? (-i % 7) : (i % 7)) * cos(radpos);
    //leds[i].y = (NUM_LEDS / 2) + (i > NUM_LEDS / 2 ? (-i % 7) : (i % 7)) * sin(radpos);
    // 7 + (0 -> i % 7 -> 0 * cos) -> 7,7

    // perentage position of the current pixel within a bounding box of it's nearest 4 pixels. 
    //px = (x - floor(x)) / abs(floor(x) - ceil(x));
    //py = (y - ceil(y)) / abs(ceil(y) - floor(y)); 

    /* Find pos & color of nearest 4 pixels counterclockwise starting with A.0 in top left, 
     * C.2 in bot right. Take the porportionate position of the pixel within this bounding 
     * box and assign each of the nearest four pixels a corresponding percentage of distance
     * away. Use these four percent distances to decide how much of each pixel to blend into
     * the current position's color.
     */
    r = g = b = 0;
    for(int j = 0; j < 4; j++) {
      int x4 = (j == 0 || j == 3) ? floor(x) : ceil(x);
      int y4 = j <= 1 ? ceil(y) : floor(y);
      percent = ((j < 1 || j > 2 ? 1 : 0) - px + (j <= 2 ? 1 : 0) - py) / 4;

      switch(imageType) {
        case PALETTE4: 
          pixNode = (x4 + y4 * NUM_LEDS) / 2;
          ptr = (uint32_t *)&imagePixels[int(pixNode)];
          p = pgm_read_byte(ptr); // Data for two pixels... [ex 0x21]
          if(pixNode == (int)pixNode) { // if whole number -> pixel #1, else pixel #2
            p >>= 4;    // Shift down 4 bits for first pixel [2 in 0x21]
          } else {
            p &= 0x0F;  // Mask out low 4 bits for second pixel [1 in 0x21]
          }
              
          r += palette[p][0] * percent;
          g += palette[p][1] * percent;
          b += palette[p][2] * percent;
          break;
      }
    }
   
    /*pixNode = (x + y * NUM_LEDS) / 2;
    ptr = (uint32_t *)&imagePixels[(int)pixNode];
    p = pgm_read_byte(ptr); // Data for two pixels... [ex 0x21]
    if(pixNode == (int)pixNode) { // if whole number -> pixel #1, else pixel #2
      p >>= 4;    // Shift down 4 bits for first pixel [2 in 0x21]
    } else {
      p &= 0x0F;  // Mask out low 4 bits for second pixel [1 in 0x21]
    }
    */
    if(i < NUM_LEDS / 2) {
      strip1.setPixelColor(i, r, g, b);
      //strip1.setPixelColor(i, palette[p][0], palette[p][1], palette[p][2]);
    } else {
      strip2.setPixelColor(i % (NUM_LEDS/2), r, g, b);
      //strip2.setPixelColor(i % (NUM_LEDS/2), palette[p][0], palette[p][1], palette[p][2]);
    }
   
    //strip1.setPixelColor(i % (NUM_LEDS/2), r, g, b);
    //strip2.setPixelColor(i % (NUM_LEDS/2), r, g, b);
  }
  
  /*switch(imageType) {
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
  } */
  strip1.show();   
  strip2.show();

  //delay((lastLoopTime - millis() + OPTIMAL_TIME) / 1000);
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

    strip1.setPixelColor(0, 255, 0, 0);
  }
} 

// handle pin change interrupt for A0 to A5
ISR (PCINT1_vect) { }


