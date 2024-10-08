#define BUTTON_PIN1 50
#define BUTTON_PIN2 51
#define BUTTON_PIN3 46
#define BUTTON_PIN4 44
#define BUTTON_PIN5 42

#define MIDI_NOTE1 36   // Base MIDI note for sensor 1
#define MIDI_NOTE2 48   // Base MIDI note for sensor 2
#define MIDI_NOTE3 60   // Base MIDI note for sensor 3
#define MIDI_NOTE4 72   // Base MIDI note for sensor 4
#define MIDI_NOTE5 84   // Base MIDI note for sensor 5

#define MIDI_CHANNEL 1 // MIDI channel (1-16)

#define LASER1_PIN 2
#define LASER2_PIN 3
#define LASER3_PIN 4
#define LASER4_PIN 5
#define LASER5_PIN 6
#define LASER6_PIN 7
#define LASER7_PIN 8
#define LASER8_PIN 9
#define LASER9_PIN 10
#define LASER10_PIN 11

#define POT1_PIN A7  // Potentiometer pin for sensor 1
#define POT2_PIN A6  // Potentiometer pin for sensor 2
#define POT3_PIN A5  // Potentiometer pin for sensor 3
#define POT4_PIN A4  // Potentiometer pin for sensor 4
#define POT5_PIN A3  // Potentiometer pin for sensor 5

struct touch {
    byte wasPressed = LOW;
    byte isPressed = LOW;
    byte lastNote = 0;   // Store the last played note to turn it off properly
};

touch touch1, touch2, touch3, touch4, touch5;

void setup() {

    //set up all touch sensors
    pinMode(BUTTON_PIN1, INPUT);
    pinMode(BUTTON_PIN2, INPUT);
    pinMode(BUTTON_PIN3, INPUT);
    pinMode(BUTTON_PIN4, INPUT);
    pinMode(BUTTON_PIN5, INPUT);

    //set all laser pins
    pinMode(LASER1_PIN, OUTPUT);
    pinMode(LASER2_PIN, OUTPUT);
    pinMode(LASER3_PIN, OUTPUT);
    pinMode(LASER4_PIN, OUTPUT);
    pinMode(LASER5_PIN, OUTPUT);
    pinMode(LASER6_PIN, OUTPUT);
    pinMode(LASER7_PIN, OUTPUT);
    pinMode(LASER8_PIN, OUTPUT);
    pinMode(LASER9_PIN, OUTPUT);
    pinMode(LASER10_PIN, OUTPUT);

    Serial.begin(115200);
}

void loop() {

    //loop just runs the checkTouch function which handles sensor touch
    checkTouch(BUTTON_PIN1, &touch1, MIDI_NOTE1, LASER1_PIN, LASER6_PIN, POT1_PIN);
    checkTouch(BUTTON_PIN2, &touch2, MIDI_NOTE2, LASER2_PIN, LASER7_PIN, POT2_PIN);
    checkTouch(BUTTON_PIN3, &touch3, MIDI_NOTE3, LASER3_PIN, LASER8_PIN, POT3_PIN);
    checkTouch(BUTTON_PIN4, &touch4, MIDI_NOTE4, LASER4_PIN, LASER9_PIN, POT4_PIN);
    checkTouch(BUTTON_PIN5, &touch5, MIDI_NOTE5, LASER5_PIN, LASER10_PIN, POT5_PIN);
}

void checkTouch(int pin, touch *t, byte baseNote, int laserPin1, int laserPin2, int potPin) {

    t->isPressed = isTouchPressed(pin);

    if (t->wasPressed != t->isPressed) {
        if (t->isPressed == HIGH) {
            //read the potentiometer value and adjust the pitch before note is played 
            byte adjustedNote = getAdjustedPitch(baseNote, potPin);

            //send the adjusted note and play laser
            t->lastNote = adjustedNote;  
            playNoteAndFlashLasers(adjustedNote, laserPin1, laserPin2, true);
        } else {
            //stop playing the last played note and turn lasers off
            playNoteAndFlashLasers(t->lastNote, laserPin1, laserPin2, false);
        }
    }

    t->wasPressed = t->isPressed;
}

bool isTouchPressed(int pin) {
    return digitalRead(pin) == HIGH;
}

void playNoteAndFlashLasers(byte note, int laserPin1, int laserPin2, bool playNote) {
    if (playNote) {
        //play the  note and turn lasers on
        midiSend(144, note, 120); 
        digitalWrite(laserPin1, HIGH);
        digitalWrite(laserPin2, HIGH);
    } else {
        // Stop the note and turn lasers off
        midiSend(128, note, 0);  
        digitalWrite(laserPin1, LOW);
        digitalWrite(laserPin2, LOW);
    }
}

//midi function to send the note 
void midiSend(byte cc_code, byte control_num, byte value) {
    Serial.flush();
    Serial.write(cc_code);
    Serial.write(control_num);
    Serial.write(value);
}

//get the new pitch based on pentiometer 
byte getAdjustedPitch(byte baseNote, int potPin) {
    int potValue = analogRead(potPin);  //read pentiometer 
    int pitchRange = 12;  //adjust pitch range by 12 which is 1 octave 
    int adjustedNote = map(potValue, 0, 1023, baseNote - pitchRange, baseNote + pitchRange);
    return constrain(adjustedNote, 0, 127);  //make sure note stays within midi range 
}
