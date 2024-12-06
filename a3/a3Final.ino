#include <Adafruit_NeoPixel.h>

#define MATRIX_PIN 2
#define MATRIX_PIN2 3
#define MATRIX_PIN3 4
#define MATRIX_PIN4 5


#define RGB_STRIP_PIN 6

#define MATRIX_PIXELS 64
#define STRIP_PIXELS 60
#define HEADER '<'
#define FOOTER '>'

#define LASER_PIN1 31
#define LASER_PIN2 29
#define LASER_PIN3 35
#define LASER_PIN4 37
#define LASER_PIN5 39
#define LASER_PIN6 41
#define LASER_PIN7 43
#define LASER_PIN8 45
#define LASER_PIN9 28
#define LASER_PIN10 30
#define LASER_PIN11 32
#define LASER_PIN12 34

#define LASER_PIN13 36
#define LASER_PIN14 26

#define OUTER_PUMPS 8
#define INNER_PUMPS 9



Adafruit_NeoPixel matrix(MATRIX_PIXELS, MATRIX_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel matrix2(MATRIX_PIXELS, MATRIX_PIN2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel matrix3(MATRIX_PIXELS, MATRIX_PIN3, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel matrix4(MATRIX_PIXELS, MATRIX_PIN4, NEO_GRB + NEO_KHZ800);


Adafruit_NeoPixel rgbStrip(STRIP_PIXELS, RGB_STRIP_PIN, NEO_GRB + NEO_KHZ800);

byte buffer[197];  // 192 for matrix + 3 for RGB strip + 1 for laser + 1 for pumps (we use custom commands to control the pumps)
boolean isReading = false;
int index = 0;

void setup() {
  Serial.begin(57600);
  matrix.begin();
  matrix2.begin();
  matrix3.begin();
  matrix4.begin();


  matrix.show();
  matrix2.show();
  matrix3.show(); 
  matrix4.show(); 

  rgbStrip.begin();
  rgbStrip.show();


  // Setup laser pins as outputs
  pinMode(LASER_PIN1, OUTPUT);
  pinMode(LASER_PIN2, OUTPUT);
  pinMode(LASER_PIN3, OUTPUT);
  pinMode(LASER_PIN4, OUTPUT);
  pinMode(LASER_PIN5, OUTPUT);
  pinMode(LASER_PIN6, OUTPUT);
  pinMode(LASER_PIN7, OUTPUT);
  pinMode(LASER_PIN8, OUTPUT);
  pinMode(LASER_PIN9, OUTPUT);
  pinMode(LASER_PIN10, OUTPUT);
  pinMode(LASER_PIN11, OUTPUT);
  pinMode(LASER_PIN12, OUTPUT);
  pinMode(LASER_PIN13, OUTPUT);
  pinMode(LASER_PIN14, OUTPUT);


  pinMode(OUTER_PUMPS, OUTPUT);
  pinMode(INNER_PUMPS, OUTPUT);
  digitalWrite(OUTER_PUMPS, HIGH);
  digitalWrite(INNER_PUMPS, HIGH);

}

void loop() {
  while (Serial.available() > 0) {
    byte inByte = Serial.read();
    

    //vixen sends its data as packets
    //In my vixen setup, the beginning of a packet begins with: < 
    if (inByte == HEADER) {
      isReading = true;
      index = 0;
      continue;
    }
    
    //once it receives the end packet value (>), then Vixen has sent over a full buffer and we can now process what was in that chunk of data
    if (inByte == FOOTER) {

      //this check has to be added to ensure the Arduino has received the entire data, otherwise some data may be lost which causes glitches in the lighting 
      if (index == 197) {
        processBuffer();
      }
      isReading = false;
      continue;
    }
    
    if (isReading && index < sizeof(buffer)) {
      buffer[index++] = inByte;
    }
  }
}

void processBuffer() {

  
  //update matrix (first 192 channels)
  for (int i = 0; i < MATRIX_PIXELS; i++) {
    int base = i * 3;
    matrix.setPixelColor(i, buffer[base], buffer[base + 1], buffer[base + 2]);
    matrix2.setPixelColor(i, buffer[base], buffer[base + 1], buffer[base + 2]);
    matrix3.setPixelColor(i, buffer[base], buffer[base + 1], buffer[base + 2]);
    matrix4.setPixelColor(i, buffer[base], buffer[base + 1], buffer[base + 2]);

  }
  matrix.show();
  matrix2.show();
  matrix3.show(); 
  matrix4.show(); 
  
  //update RGB strip using just the first 3 channels after matrix data
  //we only need 3 channels as the rest of the LEDs in the strip will be the same as the 1st LED. 
  //this saves channels in the arduino! 
  int stripBase = MATRIX_PIXELS * 3;
  uint32_t stripColor = rgbStrip.Color(buffer[stripBase], buffer[stripBase + 1], buffer[stripBase + 2]);
  rgbStrip.fill(stripColor, 0, STRIP_PIXELS);
  rgbStrip.show();


  //update lasers using single channel after RGB strip data
  int laserBase = stripBase + 3;
  byte laserValue = buffer[laserBase];
  digitalWrite(LASER_PIN1, laserValue > 127 ? HIGH : LOW);  // On if value > 127, other we keep it off
  digitalWrite(LASER_PIN2, laserValue > 127 ? HIGH : LOW);
  digitalWrite(LASER_PIN3, laserValue > 127 ? HIGH : LOW);
  digitalWrite(LASER_PIN4, laserValue > 127 ? HIGH : LOW);
  digitalWrite(LASER_PIN5, laserValue > 127 ? HIGH : LOW);
  digitalWrite(LASER_PIN6, laserValue > 127 ? HIGH : LOW);
  digitalWrite(LASER_PIN7, laserValue > 127 ? HIGH : LOW);
  digitalWrite(LASER_PIN8, laserValue > 127 ? HIGH : LOW);
  digitalWrite(LASER_PIN9, laserValue > 127 ? HIGH : LOW);
  digitalWrite(LASER_PIN10, laserValue > 127 ? HIGH : LOW);
  digitalWrite(LASER_PIN11, laserValue > 127 ? HIGH : LOW);
  digitalWrite(LASER_PIN12, laserValue > 127 ? HIGH : LOW);
  digitalWrite(LASER_PIN13, laserValue > 127 ? HIGH : LOW);
  digitalWrite(LASER_PIN14, laserValue > 127 ? HIGH : LOW);


  //pump channel - use laser to simlulate action of turning the pump on 
  int pumpBase = laserBase + 1;


  //receive the custom 8 bit value from Vixen. If it receives 3, then both sets of pumps turn on at the same time.
  byte pumpValue = buffer[pumpBase];
  digitalWrite(OUTER_PUMPS, buffer[pumpBase] == 1 || pumpValue == 3 ? LOW : HIGH);
  digitalWrite(INNER_PUMPS, buffer[pumpBase] == 2 || pumpValue == 3 ? LOW : HIGH);


}


