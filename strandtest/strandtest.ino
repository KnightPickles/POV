#include <Adafruit_DotStar.h>
// Because conditional #includes don't work w/Arduino sketches...
#include <SPI.h>         // COMMENT OUT THIS LINE FOR GEMMA OR TRINKET
#include <avr/power.h> // ENABLE THIS LINE FOR GEMMA OR TRINKET
#include <avr/sleep.h>

#define NUMPIXELS 7 // Number of LEDs in strip

#define IRPIN       10
#define HALLPIN     8
#define DATAPIN1    3
#define CLOCKPIN1   4
#define DATAPIN2    5
#define CLOCKPIN2   6

Adafruit_DotStar strip1 = Adafruit_DotStar(
  NUMPIXELS, DATAPIN1, CLOCKPIN1, DOTSTAR_BRG);

Adafruit_DotStar strip2 = Adafruit_DotStar(
  NUMPIXELS, DATAPIN2, CLOCKPIN2, DOTSTAR_BRG);

volatile byte revolutions;
volatile byte revolutionTimer;
volatile byte revolutionTime;
volatile byte revolutionDelta;
volatile bool hallDetected;

int      head  = 0, tail = -10; // Index of first 'on' and 'off' pixels
uint32_t color = 0xFF0000;      // 'On' color (starts red)

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif
  strip1.begin(); // Initialize pins for output
  strip2.begin();
  strip1.show();  // Turn all LEDs off 
  strip2.show();

  hallDetected = false;
  revolutions = revolutionTimer = revolutionTime = revolutionDelta = millis();

  // Enable specialized pin intterupts 
  //enableInterruptPin(HALLPIN);
  //enableInterruptPin(IRPIN);
}

void loop() {
  // if idling write code before interrupt sleep
  //interruptSleep();
  // normal looping code after interrupt sleep

  revolutionTime = revolutionTimer;
  revolutionTimer = millis();
  revolutionDelta = revolutionTimer - revolutionTime;
  
  if(digitalRead(HALLPIN) == LOW) {
    revolutions++;
    //revolutionDelta = revolutionTimer;
    //revolutionTimer = 0;
    /*for(int i = 0; i < NUMPIXELS; i++) {
      strip1.setPixelColor(i, 0xFF0000);
      strip2.setPixelColor(i, 0);
    }
    
    delay(revolutionDelta / 2 * 1000);
    strip1.show();
    strip2.show();
  }*/ 

  /*for(int i = 0; i < NUMPIXELS; i++) {
      strip2.setPixelColor(i, 0xFF0000);
      strip1.setPixelColor(i, 0);
  }*/

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


// ------ Intterupt Functionality ----- //
void enableInterruptPin(byte pin) {
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

void interruptSleep() {
  // The ATmega328 has five different sleep states.
  // SLEEP_MODE_IDLE -the least power savings 
  // SLEEP_MODE_ADC
  // SLEEP_MODE_PWR_SAVE
  // SLEEP_MODE_STANDBY
  // SLEEP_MODE_PWR_DOWN 
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set sleep mode.
  sleep_enable(); // Enable sleep mode.
  sleep_mode(); // Enter sleep mode.
  // Point of interrupt waking and resuming instruction execution
  sleep_disable(); // Disable sleep mode after waking.
}

ISR (PCINT0_vect) { // handle pin change interrupt for D8 to D13
  revolutions++;
  revolutionDelta = revolutionTimer;
  revolutionTimer = 0;
  if(digitalRead(HALLPIN) == LOW) {
    hallDetected = false;
  } else {
    hallDetected = true;
  }
} 

ISR (PCINT1_vect) { // handle pin change interrupt for A0 to A5
}

ISR (PCINT2_vect) { // handle pin change interrupt for D0 to D7 
}
