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
CRGB currentColor = CRGB(0, 0, 255);  

//variables for timing control
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


  //The relay is a low-active relay
  //So the HIGH command is what keeps them off and vice versa
  digitalWrite(relay5VPumps, HIGH);    
  digitalWrite(relay12VPumps, HIGH);  
  
  //38400 allows for faster communication instead of 9600bps 
  Serial.begin(38400);

  //set initial LED color to blue
  fill_solid(leds, NUM_LEDS, currentColor);
  FastLED.show();
}

void loop() {
  unsigned long currentTime = millis();
  
  //turn off laser if duration has elapsed
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
      //turn lasers on for specified duration
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
      //set RGB LED color
      while (Serial.available() < 3) { /* wait */ }
      
      byte r = Serial.read();
      byte g = Serial.read();
      byte b = Serial.read();
      
      currentColor = CRGB(r, g, b);
      fill_solid(leds, NUM_LEDS, currentColor);
      FastLED.show();
    }

    else if (command == 'C') { 
      //wait for the RGB values
      while (Serial.available() < 3) { /* wait */ }
      
      byte flashR = Serial.read();
      byte flashG = Serial.read();
      byte flashB = Serial.read();
      
      //save current color
      CRGB originalColor = currentColor;
      
      //set flash color
      //I do a flash color as I don't want the rgb strip playing with the melody during pump commands
      //the reason for this is simple: without it, the pumps lag behind..
      fill_solid(leds, NUM_LEDS, CRGB(flashR, flashG, flashB));
      FastLED.show();
      
      //pump the pumps
      digitalWrite(relay5VPumps, LOW);  
      digitalWrite(relay12VPumps, LOW);   

      delay(1000);  
      digitalWrite(relay5VPumps, HIGH);  
      digitalWrite(relay12VPumps, HIGH); 


      //return to original color after flash
      fill_solid(leds, NUM_LEDS, originalColor);
      FastLED.show();
    }
  
  }
}