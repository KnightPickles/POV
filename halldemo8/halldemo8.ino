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

#define NUM_STRIPS  8
#define NUM_ST_LEDS 14
#define NUM_LEDS 56

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

/*
Adafruit_DotStar LED_A1 = Adafruit_DotStar(NUM_ST_LEDS, DATA_A1, CLOCK_A, DOTSTAR_BGR);
Adafruit_DotStar LED_A2 = Adafruit_DotStar(NUM_ST_LEDS, DATA_A2, CLOCK_A, DOTSTAR_BGR);
Adafruit_DotStar LED_A3 = Adafruit_DotStar(NUM_ST_LEDS, DATA_A3, CLOCK_A, DOTSTAR_BGR);
Adafruit_DotStar LED_A4 = Adafruit_DotStar(NUM_ST_LEDS, DATA_A4, CLOCK_A, DOTSTAR_BGR);

Adafruit_DotStar LED_B1 = Adafruit_DotStar(NUM_ST_LEDS, DATA_B1, CLOCK_B, DOTSTAR_BGR);
Adafruit_DotStar LED_B2 = Adafruit_DotStar(NUM_ST_LEDS, DATA_B2, CLOCK_B, DOTSTAR_BGR);
Adafruit_DotStar LED_B3 = Adafruit_DotStar(NUM_ST_LEDS, DATA_B3, CLOCK_B, DOTSTAR_BGR);
Adafruit_DotStar LED_B4 = Adafruit_DotStar(NUM_ST_LEDS, DATA_B4, CLOCK_B, DOTSTAR_BGR);*/

Adafruit_DotStar LED_A1 = Adafruit_DotStar(NUM_LEDS, DATA_A1, CLOCK_A, DOTSTAR_BGR);
Adafruit_DotStar LED_B1 = Adafruit_DotStar(NUM_LEDS, DATA_B1, CLOCK_B, DOTSTAR_BGR);

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif
  LED_A1.begin();
  //LED_A2.begin();
  //LED_A3.begin();
  //LED_A4.begin();
  LED_B1.begin();
  //LED_B2.begin();
  //LED_B3.begin();
  //LED_B4.begin();

  LED_A1.clear();
  //LED_A2.clear();
  //LED_A3.clear();
  //LED_A4.clear();
  LED_B1.clear();
  //LED_B2.clear();
  //LED_B3.clear();
  //LED_B4.clear();

  LED_A1.show();
  //LED_A2.show();
  //LED_A3.show();
  //LED_A4.show();
  LED_B1.show();
  //LED_B2.show();
  //LED_B3.show();
  //LED_B4.show();
}


int      head  = 0, tail = -10; // Index of first 'on' and 'off' pixels
int      head2  = 0, tail2 = -10; // Index of first 'on' and 'off' pixels
uint32_t color = 0xFF0000;      // 'On' color (starts red)

void loop() {
  if(digitalRead(HALLPIN) == LOW) {
    LED_A1.setPixelColor(head, color);
    LED_A1.setPixelColor(tail, 0);
     LED_A1.setPixelColor(head, color);
    LED_A1.setPixelColor(tail, 0);
    /*LED_A2.setPixelColor(head, color);
    LED_A2.setPixelColor(tail, 0);
    LED_A3.setPixelColor(head, color);
    LED_A3.setPixelColor(tail, 0);
    LED_A4.setPixelColor(head, color);
    LED_A4.setPixelColor(tail, 0);*/
    LED_B1.setPixelColor(head, color);
    LED_B1.setPixelColor(tail, 0);
     LED_B1.setPixelColor(head, color);
    LED_B1.setPixelColor(tail, 0);
    /*LED_B2.setPixelColor(head, color);
    LED_B2.setPixelColor(tail, 0);
    LED_B3.setPixelColor(head, color);
    LED_B3.setPixelColor(tail, 0);
    LED_B4.setPixelColor(head, color);
    LED_B4.setPixelColor(tail, 0);*/
               
    LED_A1.show();
    //LED_A2.show();
    //LED_A3.show();
    //LED_A4.show();
    LED_B1.show();
    //LED_B2.show();
    //LED_B3.show();
    //LED_B4.show();                    // Refresh strip
  
    if(++head >= NUM_LEDS/2) {         // Increment head index.  Off end of strip?
      head = 0;                       //  Yes, reset head index to start
      if((color >>= 8) == 0)          //  Next color (R->G->B) ... past blue now?
        color = 0xFF0000;             //   Yes, reset to red
    }
    if(++tail >= NUM_LEDS/2) tail = 0; // Increment, reset tail indezx

    if(++head2 >= NUM_LEDS) {         // Increment head index.  Off end of strip?
      head2 = NUM_LEDS/2;                       //  Yes, reset head index to start
      if((color >>= 8) == 0)          //  Next color (R->G->B) ... past blue now?
        color = 0xFF0000;             //   Yes, reset to red
    }
    if(++tail2 >= NUM_LEDS) tail2 = NUM_LEDS/2; // Increment, reset tail indezx
  }
}
