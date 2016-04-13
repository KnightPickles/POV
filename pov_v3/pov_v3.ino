/*------------------------------------------------------------------------
  POV LED double staff sketch.  Uses the following Adafruit parts
  (X2 for two staffs):

  - Pro Trinket 5V https://www.adafruit.com/product/2000
  - 2200 mAh Lithium Ion Battery https://www.adafruit.com/product/1781
  - LiPoly Backpack https://www.adafruit.com/product/2124
  - Tactile On/Off Switch with Leads https://www.adafruit.com/product/1092
  - 144 LED/m DotStar strip (#2328 or #2329)
    (ONE METER is enough for ONE STAFF, TWO METERS for TWO staffs)
  - Infrared Sensor: https://www.adafruit.com/product/157
  - Mini Remote Control: https://www.adafruit.com/product/389
    (only one remote is required for multiple staffs)

  Needs Adafruit_DotStar library: github.com/adafruit/Adafruit_DotStar

  This is based on the LED poi code (also included in the repository),
  but ATtiny-specific code has been stripped out for brevity, since the
  staffs pretty much require Pro Trinket or better (lots more LEDs here).

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Phil Burgess / Paint Your Dragon for Adafruit Industries.
  MIT license, all text above must be included in any redistribution.
  See 'COPYING' file for additional notes.
  ------------------------------------------------------------------------*/

#include <Arduino.h>
#include <Adafruit_DotStar.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <SPI.h>

typedef uint16_t line_t;

// CONFIGURABLE STUFF ------------------------------------------------------

#include "letter14p.h" // Graphics data is contained in this header file.
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
#define LED_DATA_PIN  3
#define LED_CLOCK_PIN 4
#define HALLPIN     8
#define DATAPIN1    3
#define CLOCKPIN1   4
#define DATAPIN2    5
#define CLOCKPIN2   6

// -------------------------------------------------------------------------

#if defined(LED_DATA_PIN) && defined(LED_CLOCK_PIN)
// Older DotStar LEDs use GBR order.  If colors are wrong, edit here.
Adafruit_DotStar strip1 = Adafruit_DotStar(NUM_LEDS,
  DATAPIN1, CLOCKPIN1, DOTSTAR_BGR);
Adafruit_DotStar strip2 = Adafruit_DotStar(NUM_LEDS,
  DATAPIN2, CLOCKPIN2, DOTSTAR_BGR);
#else
Adafruit_DotStar strip1 = Adafruit_DotStar(NUM_LEDS, DOTSTAR_BGR); 
Adafruit_DotStar strip2 = Adafruit_DotStar(NUM_LEDS, DOTSTAR_BGR); 
#endif

void     imageInit(void);

void setup() {
  strip1.begin(); // Allocate DotStar buffer, init SPI
  strip2.begin();
  strip1.clear(); // Make sure strip is clear
  strip2.clear();
  strip1.show();  // before measuring battery
  strip2.show();

  imageInit();   // Initialize pointers for default image
  enableInterruptPin(HALLPIN);
}

// GLOBAL STATE STUFF ------------------------------------------------------
volatile uint32_t rps, // revolution per second
                  revolutions,
                  revolutionDelta, // Time of a single revolution
                  rpsAccumulator, // Accumulates individual revolution time for calculating rps 
                  hallStart; // The time in millis when the hall sensor was last detected

float x, y, px, py, percent, radPos, degPos, pi = 3.14159;
int pideg = 180, pi2deg = 360, pi3deg = pideg + pi2deg;


uint32_t lastImageTime = 0L, // Time of last image change
         lastLineTime  = 0L;
uint8_t  imageNumber   = 0,  // Current image being displayed
         imageType,          // Image type: PALETTE[1,4,8] or TRUECOLOR
        *imagePalette,       // -> palette data in PROGMEM
        *imagePixels,        // -> pixel data in PROGMEM
         palette[16][3];     // RAM-based color table for 1- or 4-bit images
line_t   imageLines,         // Number of lines in active image
         imageLine1,          // Current line number in image
         imageLine2;

// Microseconds per line for various speed settings
const uint16_t PROGMEM lineTable[] = { // 375 * 2^(n/3)
  1000000L /  375, // 375 lines/sec = slowest
  1000000L /  472,
  1000000L /  595,
  1000000L /  750, // 750 lines/sec = mid
  1000000L /  945,
  1000000L / 1191,
  1000000L / 1500  // 1500 lines/sec = fastest
};
uint8_t  lineIntervalIndex = 3;
uint16_t lineInterval      = 1000000L / 750;

void imageInit() { // Initialize global image state for current imageNumber
  imageType    = pgm_read_byte(&images[imageNumber].type);
  imageLines   = pgm_read_word(&images[imageNumber].lines);
  imageLine1   = 0;
  imageLine2   = 0;
  imagePalette = (uint8_t *)pgm_read_word(&images[imageNumber].palette);
  imagePixels  = (uint8_t *)pgm_read_word(&images[imageNumber].pixels);
  // 1- and 4-bit images have their color palette loaded into RAM both for
  // faster access and to allow dynamic color changing.  Not done w/8-bit
  // because that would require inordinate RAM (328P could handle it, but
  // I'd rather keep the RAM free for other features in the future).
  if(imageType == PALETTE1)      memcpy_P(palette, imagePalette,  2 * 3);
  else if(imageType == PALETTE4) memcpy_P(palette, imagePalette, 16 * 3);
  lastImageTime = millis(); // Save time of image init for next auto-cycle
}

void nextImage(void) {
  if(++imageNumber >= NUM_IMAGES) imageNumber = 0;
  imageInit();
}

void prevImage(void) {
  imageNumber = imageNumber ? imageNumber - 1 : NUM_IMAGES - 1;
  imageInit();
}

// MAIN LOOP ---------------------------------------------------------------

void loop() {
  uint32_t t = millis(); // Current time, milliseconds
  degPos = ((millis() - hallStart) * pi2deg) / revolutionDelta; 
  imageLine1 = (imageLines * degPos) / pi2deg;
  imageLine2 = (imageLines * (degPos + pideg)) / pi2deg;
  if(imageLine2 > imageLines) imageLine2 -= imageLines; // wrap
  

  // Transfer one scanline from pixel data to LED strip:

  // If you're really pressed for graphics space and need just a few extra
  // scanlines, and know for a fact you won't be using certain image modes,
  // you can comment out the corresponding blocks below.  e.g. disabling
  // PALETTE8 and TRUECOLOR support can free up nearly 200 bytes of extra
  // image storage.

  switch(imageType) {

    case PALETTE1: { // 1-bit (2 color) palette-based image
      uint8_t  pixelNum = 0, byteNum, bitNum, pixels, idx,
              *ptr = (uint8_t *)&imagePixels[imageLine1 * NUM_LEDS / 8];
      for(byteNum = NUM_LEDS/8; byteNum--; ) { // Always padded to next byte
        pixels = pgm_read_byte(ptr++);  // 8 pixels of data (pixel 0 = LSB)
        for(bitNum = 8; bitNum--; pixels >>= 1) {
          idx = pixels & 1; // Color table index for pixel (0 or 1)
          strip1.setPixelColor(pixelNum++,
            palette[idx][0], palette[idx][1], palette[idx][2]);
        }
      }

      pixelNum = 0;
      ptr = (uint8_t *)&imagePixels[imageLine2 * NUM_LEDS / 8];
      for(byteNum = NUM_LEDS/8; byteNum--; ) { // Always padded to next byte
        pixels = pgm_read_byte(ptr++);  // 8 pixels of data (pixel 0 = LSB)
        for(bitNum = 8; bitNum--; pixels >>= 1) {
          idx = pixels & 1; // Color table index for pixel (0 or 1)
          strip2.setPixelColor(pixelNum++,
            palette[idx][0], palette[idx][1], palette[idx][2]);
        }
      }
      break;
    }

    case PALETTE4: { // 4-bit (16 color) palette-based image
      uint8_t  pixelNum, p1, p2,
              *ptr = (uint8_t *)&imagePixels[imageLine1 * NUM_LEDS / 2];
      for(pixelNum = 0; pixelNum < NUM_LEDS; ) {
        p2  = pgm_read_byte(ptr++); // Data for two pixels...
        p1  = p2 >> 4;              // Shift down 4 bits for first pixel
        p2 &= 0x0F;                 // Mask out low 4 bits for second pixel
        strip1.setPixelColor(pixelNum++,
          palette[p1][0], palette[p1][1], palette[p1][2]);
        strip1.setPixelColor(pixelNum++,
          palette[p2][0], palette[p2][1], palette[p2][2]);
      }

      ptr = (uint8_t *)&imagePixels[imageLine2 * NUM_LEDS / 2];
      for(pixelNum = 0; pixelNum < NUM_LEDS; ) {
        p2  = pgm_read_byte(ptr++); // Data for two pixels...
        p1  = p2 >> 4;              // Shift down 4 bits for first pixel
        p2 &= 0x0F;                 // Mask out low 4 bits for second pixel
        strip2.setPixelColor(pixelNum++,
          palette[p1][0], palette[p1][1], palette[p1][2]);
        strip2.setPixelColor(pixelNum++,
          palette[p2][0], palette[p2][1], palette[p2][2]);
      }
      break;
    }

    case PALETTE8: { // 8-bit (256 color) PROGMEM-palette-based image
      uint16_t  o;
      uint8_t   pixelNum,
               *ptr = (uint8_t *)&imagePixels[imageLine1 * NUM_LEDS];
      for(pixelNum = 0; pixelNum < NUM_LEDS; pixelNum++) {
        o = pgm_read_byte(ptr++) * 3; // Offset into imagePalette
        strip1.setPixelColor(pixelNum,
          pgm_read_byte(&imagePalette[o]),
          pgm_read_byte(&imagePalette[o + 1]),
          pgm_read_byte(&imagePalette[o + 2]));
      }
      
      ptr = (uint8_t *)&imagePixels[imageLine2 * NUM_LEDS];
      for(pixelNum = 0; pixelNum < NUM_LEDS; pixelNum++) {
        o = pgm_read_byte(ptr++) * 3; // Offset into imagePalette
        strip2.setPixelColor(pixelNum,
          pgm_read_byte(&imagePalette[o]),
          pgm_read_byte(&imagePalette[o + 1]),
          pgm_read_byte(&imagePalette[o + 2]));
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
        strip1.setPixelColor(pixelNum, r, g, b);
      }

      ptr = (uint8_t *)&imagePixels[imageLine2 * NUM_LEDS * 3];
      for(pixelNum = 0; pixelNum < NUM_LEDS; pixelNum++) {
        r = pgm_read_byte(ptr++);
        g = pgm_read_byte(ptr++);
        b = pgm_read_byte(ptr++);
        strip2.setPixelColor(pixelNum, r, g, b);
      }
      break;
    }
  }

  //if(++imageLine >= imageLines) imageLine = 0; // Next scanline, wrap around

  strip1.show(); // Refresh LEDs
  strip2.show();
  lastLineTime = t;
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


