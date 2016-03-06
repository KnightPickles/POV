#include <Adafruit_DotStar.h>
// Because conditional #includes don't work w/Arduino sketches...
#include <SPI.h>         // COMMENT OUT THIS LINE FOR GEMMA OR TRINKET
#include <avr/power.h> // ENABLE THIS LINE FOR GEMMA OR TRINKET
#include <avr/sleep.h>

#define NUMLEDS 7 // Number of LEDs in strip

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

volatile unsigned byte rps;  
volatile unsigned byte revolutions;
volatile unsigned byte revolutionDelta; // Time of a single revolution
volatile unsigned int  revolutionTime; 
volatile unsigned long int rpsAccumulator; // Accumulates individual revolution time for calculating rps 
volatile unsigned long int hallStart;

//int      head  = 0, tail = -10; // Index of first 'on' and 'off' pixels
//uint32_t color = 0xFF0000;      // 'On' color (starts red)

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
  rps = rpsAccumulator = revolutions = revolutionTime = 0; 

  // Enable specialized pin intterupts 
  enableInterruptPin(HALLPIN);
  //enableInterruptPin(IRPIN);
}

void loop() {
  // 
  if(millis() <= revolutionTime) { 
    for(int i = 0; i < NUMLEDS; i++) {
      strip1.setPixelColor(i, 0xFF00FF); 
      strip2.setPixelColor(i, 0x00FF00);     
    }
  } else {
    for(int i = 0; i < NUMLEDS; i++) {
      strip2.setPixelColor(i, 0xFF00FF);
      strip1.setPixelColor(i, 0x00FF00); 
    }
  }

  // psudo rps indicator to let me know when the prototype motor is overheating
  // Green is good. Purple is bad -- rps is slowing down or below average.
  if(revolutionDelta >= 170 && revolutionDelta <= 190) {
    strip1.setPixelColor(NUMLEDS - 1, 0xFF0000); // green
    strip2.setPixelColor(NUMLEDS - 1, 0xFF0000);
  } else {
    strip1.setPixelColor(NUMLEDS - 1, 0x00FFFF); // purple
    strip2.setPixelColor(NUMLEDS - 1, 0x00FFFF);
  }

  strip1.show();   
  strip2.show();
}


// ------ Intterupt Functionality ----- //
void enableInterruptPin(byte pin) {
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

// Use to put microcontroller to sleep. Wake on interrupt. 
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
  if(digitalRead(HALLPIN) == LOW) { // Hall sensor detected 
    revolutions++;
    revolutionDelta = millis() - hallStart; 
    hallStart = millis();
    revolutionTime = hallStart + (revolutionDelta / 2);
    rpsAccumulator += revolutionDelta; 
    if(rpsAccumulator >= 1000) { // Dealing in milliseconds
      rps = rpsAccumulator / 1000; 
      rpsAccumulator = 0;
    }
  }
} 

// handle pin change interrupt for A0 to A5
ISR (PCINT1_vect) { }


