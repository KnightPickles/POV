// Simple strand test for Adafruit Dot Star RGB LED strip.
// This is a basic diagnostic tool, NOT a graphics demo...helps confirm
// correct wiring and tests each pixel's ability to display red, green
// and blue and to forward data down the line.  By limiting the number
// and color of LEDs, it's reasonably safe to power a couple meters off
// the Arduino's 5V pin.  DON'T try that with other code!

#include <Adafruit_DotStar.h>
// Because conditional #includes don't work w/Arduino sketches...
#include <SPI.h>         // COMMENT OUT THIS LINE FOR GEMMA OR TRINKET
//#include <avr/power.h> // ENABLE THIS LINE FOR GEMMA OR TRINKET

#define NUMPIXELS 7 // Number of LEDs in strip

#define HALLPIN     8

// Here's how to control the LEDs from any two pins:
#define DATAPIN1    3
#define CLOCKPIN1   4
#define DATAPIN2    5
#define CLOCKPIN2   6

Adafruit_DotStar strip1 = Adafruit_DotStar(
  NUMPIXELS, DATAPIN1, CLOCKPIN1, DOTSTAR_BRG);

Adafruit_DotStar strip2 = Adafruit_DotStar(
  NUMPIXELS, DATAPIN2, CLOCKPIN2, DOTSTAR_BRG);
// The last parameter is optional -- this is the color data order of the
// DotStar strip, which has changed over time in different production runs.
// Your code just uses R,G,B colors, the library then reassigns as needed.
// Default is DOTSTAR_BRG, so change this if you have an earlier strip.

// Hardware SPI is a little faster, but must be wired to specific pins
// (Arduino Uno = pin 11 for data, 13 for clock, other boards are different).
//Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DOTSTAR_BRG);

volatile byte revolutions;
volatile bool detected;

void hallSensor() {
  detected = true;
}

void setup() {

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif

  strip1.begin(); // Initialize pins for output
  strip2.begin();
  strip1.show();  // Turn all LEDs off ASAP
  strip2.show();

  detected = false;
  revolutions = 0;
  //attachInterrupt(digitalPinToInterrupt(HALLPIN), hallSensor, LOW);
}

// Runs 10 LEDs at a time along strip, cycling through red, green and blue.
// This requires about 200 mA for all the 'on' pixels + 1 mA per 'off' pixel.

int      head  = 0, tail = -10; // Index of first 'on' and 'off' pixels
uint32_t color = 0xFF0000;      // 'On' color (starts red)

void loop() {

  if(digitalRead(HALLPIN) == LOW) {
    strip1.setPixelColor(head, color); // 'On' pixel at head
    strip1.setPixelColor(tail, 0);     // 'Off' pixel at tail
    strip1.show();                     // Refresh strip
    
    strip2.setPixelColor(head, color); // 'On' pixel at head
    strip2.setPixelColor(tail, 0);     // 'Off' pixel at tail
    strip2.show();          
    
    delay(20);                        // Pause 20 milliseconds (~50 FPS)
  
    if(++head >= NUMPIXELS) {         // Increment head index.  Off end of strip?
      head = 0;                       //  Yes, reset head index to start
      if((color >>= 8) == 0)          //  Next color (R->G->B) ... past blue now?
        color = 0xFF0000;             //   Yes, reset to red
    }
    if(++tail >= NUMPIXELS) tail = 0; // Increment, reset tail index
  } 
}
