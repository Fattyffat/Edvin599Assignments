#include <FastLED.h>

// LED strip configuration
#define LED_PIN     2
#define NUM_LEDS    60    
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

// Laser and pump pin definitions
const int LASER1_PIN = 8;
const int LASER2_PIN = 9;
const int LASER3_PIN = 10;
const int LASER4_PIN = 11;
const int LASER5_PIN = 12;
const int LASER6_PIN = 13;
const int LASER7_PIN = 7;
const int LASER8_PIN = 6;



const int relay5VPumps = 23;
const int relay12VPumps = 25;

CRGB leds[NUM_LEDS];
CRGB currentColor = CRGB(0, 0, 255);  // Initialize with blue (R=0, G=0, B=255)

// Variables for timing control
unsigned long laserStartTime = 0;
unsigned long laserDuration = 0;
bool laserActive = false;

void setup() {
  // LED strip setup
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(75);
  
  // Pin modes for lasers and relays
  pinMode(LASER1_PIN, OUTPUT);
  pinMode(LASER2_PIN, OUTPUT);
  pinMode(LASER3_PIN, OUTPUT);
  pinMode(LASER4_PIN, OUTPUT);
  pinMode(LASER5_PIN, OUTPUT);
  pinMode(LASER6_PIN, OUTPUT);
  pinMode(LASER7_PIN, OUTPUT);
  pinMode(LASER8_PIN, OUTPUT);
  
  pinMode(relay5VPumps, OUTPUT);
  pinMode(relay12VPumps, OUTPUT);

  digitalWrite(relay5VPumps, HIGH);    // Ensure 5V relay is off
  digitalWrite(relay12VPumps, HIGH);   // Ensure 12V relay starts off
  
  Serial.begin(38400);

  // Set initial LED color to blue
  fill_solid(leds, NUM_LEDS, currentColor);
  FastLED.show();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Turn off laser if duration has elapsed
  if (laserActive && (currentTime - laserStartTime >= laserDuration)) {
    digitalWrite(LASER1_PIN, LOW);
    digitalWrite(LASER2_PIN, LOW);
    digitalWrite(LASER3_PIN, LOW);
    digitalWrite(LASER4_PIN, LOW);
    digitalWrite(LASER5_PIN, LOW);
    digitalWrite(LASER6_PIN, LOW);
    digitalWrite(LASER7_PIN, LOW);
    digitalWrite(LASER8_PIN, LOW);
    laserActive = false;
  }

  if (Serial.available() > 0) {
    char command = Serial.read();
    
    if (command == 'L') {
      // Turn lasers on for specified duration
      while (Serial.available() < 2) { /* wait */ }
      byte durationHigh = Serial.read();
      byte durationLow = Serial.read();
      
      laserDuration = (durationHigh << 8) | durationLow;
      laserStartTime = millis();
      laserActive = true;
      
      digitalWrite(LASER1_PIN, HIGH);
      digitalWrite(LASER2_PIN, HIGH);
      digitalWrite(LASER3_PIN, HIGH);
      digitalWrite(LASER4_PIN, HIGH);
      digitalWrite(LASER5_PIN, HIGH);
      digitalWrite(LASER6_PIN, HIGH);
      digitalWrite(LASER7_PIN, HIGH);
      digitalWrite(LASER8_PIN, HIGH);
    }
    
    else if (command == 'R') {
      // Set RGB LED color
      while (Serial.available() < 3) { /* wait */ }
      
      byte r = Serial.read();
      byte g = Serial.read();
      byte b = Serial.read();
      
      currentColor = CRGB(r, g, b);
      fill_solid(leds, NUM_LEDS, currentColor);
      FastLED.show();
    }

    else if (command == 'C') {  // Combined flash and 5V pump command
      // Wait for the RGB values of the flash color
      while (Serial.available() < 3) { /* wait */ }
      
      byte flashR = Serial.read();
      byte flashG = Serial.read();
      byte flashB = Serial.read();
      
      // Save current color
      CRGB originalColor = currentColor;
      
      // Set flash color
      fill_solid(leds, NUM_LEDS, CRGB(flashR, flashG, flashB));
      FastLED.show();
      
      // Only pulse the 5V pumps
      digitalWrite(relay5VPumps, LOW);   // Activate 5V pumps
      digitalWrite(relay12VPumps, LOW);   // Turn on 12V pumps

      delay(1000);  // Duration for flash and 5V pump
      digitalWrite(relay5VPumps, HIGH);  // Deactivate 5V pumps
      digitalWrite(relay12VPumps, HIGH);  // Turn off 12V pumps


      // Return to original color
      fill_solid(leds, NUM_LEDS, originalColor);
      FastLED.show();
    }
  
  }
}