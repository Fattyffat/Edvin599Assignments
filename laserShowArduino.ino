
const int middleLaserPins[] = {2, 3, 4, 5, 6, 7, 8, 11, 12, 13}; //pins for middle lasers

const int ledPin = A5;    // LED pin

const int touchPin = 10; 
// const int buzzerPin = 11; change this one to 9 probably 

bool middleLasersDone = false; //track if the middle lasers task is done
unsigned long touchStartTime = 0; //store when the touch starts
bool isTouching = false; //track if the sensor is currently being touched

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  pinMode(touchPin, INPUT);
  // pinMode(buzzerPin, OUTPUT); 

  for (int i = 0; i < 10; i++) {
      pinMode(middleLaserPins[i], OUTPUT);
      digitalWrite(middleLaserPins[i], LOW); // Turn off initially
  }

  digitalWrite(ledPin, LOW);  // Turn off the LED initially

}

void loop() {
  int touchState = digitalRead(touchPin); 

  //sensor is touched
  if (touchState == HIGH && !isTouching) {
    middleLasersDone = false;
    isTouching = true; //start tracking touch

    touchStartTime = millis(); //record time to see how long the touch started
    
    Serial.println("Sensor Touched");
    
    //turn on all lasers immediately as soon as there is touch
    for (int i = 0; i < 10; i++) {
      digitalWrite(middleLaserPins[i], HIGH); // Turn on lasers
    }

    // Turn on the LED
    digitalWrite(ledPin, HIGH);
  
  }

  //when the sensor is released
  if (touchState == LOW && isTouching) {
    isTouching = false; //stop tracking touch
    
    Serial.println("Sensor Released");

    //calculate how long the sensor was held
    unsigned long touchDuration = millis() - touchStartTime;

    //if the touch was less than 3 seconds.. then just turn off all the lasers upon release
    if (touchDuration < 3000) {
      //turn off all lasers when released
      for (int i = 0; i < 10; i++) {
        digitalWrite(middleLaserPins[i], LOW); 
      }

      digitalWrite(ledPin, LOW);  // Turn off the LED
    }
    //if the touch duration was more than 3 seconds and the laser sequence isnt running..
    else if (touchDuration >= 3000 && !middleLasersDone) {

       
      //turn off all lasers when released to load sequence
      for (int i = 0; i < 10; i++) {
        digitalWrite(middleLaserPins[i], LOW); // Turn off lasers
      }

      //play buzzer once to notify that laser sequence is starting
      // digitalWrite(buzzerPin, HIGH);
      delay(500);
      // digitalWrite(buzzerPin, LOW);
      delay(500);

      // taskUnderControl(); 
      taskMiddleLasers(); //run the middle laser seqeuence
      middleLasersDone = true; //prevent running it immediatelly after it is done



    }
  }
}


  // Task to control middle lasers
void taskMiddleLasers() {
    while (!middleLasersDone) {

      //1:16-1:23 
      //keep them on for 7 seconds
      for (int i = 0; i < 6; i++) {
        digitalWrite(middleLaserPins[i], HIGH); // Turn on lasers
      }

      delay(7400);

      for (int i = 0; i < 6; i++) {
        digitalWrite(middleLaserPins[i], LOW); // Turn on lasers
      }

      //1:23-1:31 flash indiviaul laser randomly (I think we have to this 25 times)
      for (int i = 0; i < 27; i++) {
        int randomLaser = random(0, 6);  // Select a random laser (0 to 5)
        int randomLaser2 = random(0, 6);
    
        // Turn on the selected laser
        digitalWrite(middleLaserPins[randomLaser], HIGH);
        digitalWrite(middleLaserPins[randomLaser2], HIGH);
        delay(280);  // Keep it on for 30 milliseconds

        // Turn off the selected laser
        digitalWrite(middleLaserPins[randomLaser], LOW);
        digitalWrite(middleLaserPins[randomLaser2], LOW);
      }

      //1:31-1:38 turn off all lasers off for drop build up - edge lasers on. 
      for (int i = 0; i < 6; i++) {
        digitalWrite(middleLaserPins[i], LOW); // Turn off lasers
      }

      for (int i = 0; i < 19; i++) {


        for (int i = 6; i < 10; i++) {
          digitalWrite(middleLaserPins[i], HIGH); // Turn on lasers
        }
    
        delay(200);  // Keep it on for 45 milliseconds

        for (int i = 6; i < 10; i++) {
          digitalWrite(middleLaserPins[i], LOW); // Turn on lasers
        }

        delay(200);  // Keep them off for 420 ms before flashing again

      }

      // delay(7700);

      //1:38-1:42 flash indiviaul laser randomly super fast 
      for (int i = 0; i < 24; i++) {
        int randomLaser = random(0, 6);  // Select a random laser (0 to 5)
        int randomLaser2 = random(0, 6);  // Select a random laser (0 to 5)
      
    
        // Turn on the selected laser
        digitalWrite(middleLaserPins[randomLaser], HIGH);
        digitalWrite(middleLaserPins[randomLaser2], HIGH);
        delay(160);  // Keep it on for 20 milliseconds

        // Turn off the selected laser
        digitalWrite(middleLaserPins[randomLaser], LOW);
        digitalWrite(middleLaserPins[randomLaser2], LOW);
      }

      //1:43-1:46 flash indiviaul laser randomly super fast 
      for (int i = 0; i < 35; i++) {
        int randomLaser = random(0, 6);  // Select a random laser (0 to 5)
        int randomLaser2 = random(0, 6);  // Select a random laser (0 to 5)
    
        // Turn on the selected laser
        digitalWrite(middleLaserPins[randomLaser], HIGH);
        digitalWrite(middleLaserPins[randomLaser2], HIGH);
        delay(100);  // Keep it on for 10 milliseconds

        // Turn off the selected laser
        digitalWrite(middleLaserPins[randomLaser], LOW);
        digitalWrite(middleLaserPins[randomLaser2], LOW);

      }

      //1:46-1:53 flash all lasers same time  
      for (int i = 0; i < 27; i++) {


        for (int i = 0; i < 10; i++) {
          digitalWrite(middleLaserPins[i], HIGH); // Turn on lasers
        }
    
        delay(250);  // Keep it on for 45 milliseconds

        for (int i = 0; i < 10; i++) {
          digitalWrite(middleLaserPins[i], LOW); // Turn on lasers
        }

        delay(250);  // Keep them off for 420 ms before flashing again

      }


      //1:59-2:01 flash indiviaul laser randomly super fast 
      for (int i = 0; i < 15; i++) {
        int randomLaser = random(0, 6);  // Select a random laser (0 to 5)
    
        // Turn on the selected laser
        digitalWrite(middleLaserPins[randomLaser], HIGH);
        delay(100);  // Keep it on for 15 milliseconds

        // Turn off the selected laser
        digitalWrite(middleLaserPins[randomLaser], LOW);

      }


      //2:02-2:14 flash all lasers together 
      for (int i = 0; i < 27; i++) {

        for (int i = 0; i < 10; i++) {
          digitalWrite(middleLaserPins[i], HIGH); // Turn on lasers
        }
    
        delay(250);  // Keep it on for 45 milliseconds

        for (int i = 0; i < 10; i++) {
          digitalWrite(middleLaserPins[i], LOW); // Turn on lasers
        }

        delay(250);  // Keep it on for 45 milliseconds

      }


      //2:14-2:17 flash indiviaul laser randomly super fast 
      for (int i = 0; i < 22; i++) {
        int randomLaser = random(0, 6);  // Select a random laser (0 to 5)
    
        // Turn on the selected laser
        digitalWrite(middleLaserPins[randomLaser], HIGH);
        delay(75);  // Keep it on for 15 milliseconds

        // Turn off the selected laser
        digitalWrite(middleLaserPins[randomLaser], LOW);

      }

      for (int i = 0; i < 10; i++) {
        digitalWrite(middleLaserPins[i], HIGH); // Turn on lasers
      }

      delay(3000);

      for (int i = 0; i < 10; i++) {
        digitalWrite(middleLaserPins[i], LOW); // Turn off lasers
      }


      middleLasersDone = true; 
      


    }
  }

  void taskUnderControl() {

      //UNDER CONTROL
      delay(2000);


      //2:26 
      //all on
      for (int i = 0; i < 10; i++) {
        digitalWrite(middleLaserPins[i], HIGH); // Turn on lasers
      }

      delay(2000);

      //edge lasers - 2:28-30
      for (int i = 0; i < 4; i++) {


        for (int i = 6; i < 10; i++) {
          digitalWrite(middleLaserPins[i], HIGH); // Turn on lasers
        }
    
        delay(250);  // Keep it on for 45 milliseconds

        for (int i = 6; i < 10; i++) {
          digitalWrite(middleLaserPins[i], LOW); // off on lasers
        }

        delay(250);  // Keep them off for 420 ms before flashing again

      }

      delay(2000); 

      //2:31-2:33
      for (int i = 0; i < 4; i++) {


        for (int i = 6; i < 10; i++) {
          digitalWrite(middleLaserPins[i], HIGH); // Turn on lasers
        }
    
        delay(250);  // Keep it on for 45 milliseconds

        for (int i = 6; i < 10; i++) {
          digitalWrite(middleLaserPins[i], LOW); // off on lasers
        }

        delay(250);  // Keep them off for 420 ms before flashing again

      }


      //2:33-2:37 flash indiviaul laser randomly super fast 
      for (int i = 0; i < 26; i++) {
        int randomLaser = random(0, 6);  // Select a random laser (0 to 5)
        int randomLaser2 = random(0, 6);  // Select a random laser (0 to 5)
    
        // Turn on the selected laser
        digitalWrite(middleLaserPins[randomLaser], HIGH);
        digitalWrite(middleLaserPins[randomLaser2], HIGH);
        delay(110);  // Keep it on for 10 milliseconds

        // Turn off the selected laser
        digitalWrite(middleLaserPins[randomLaser], LOW);
        digitalWrite(middleLaserPins[randomLaser2], LOW);

      }

      //2:38-2:40
      for (int i = 0; i < 74; i++) {
        int randomLaser = random(0, 6);  // Select a random laser (0 to 5)
        int randomLaser2 = random(0, 6);  // Select a random laser (0 to 5)
    
        // Turn on the selected laser
        digitalWrite(middleLaserPins[randomLaser], HIGH);
        digitalWrite(middleLaserPins[randomLaser2], HIGH);
        delay(60);  // Keep it on for 10 milliseconds

        // Turn off the selected laser
        digitalWrite(middleLaserPins[randomLaser], LOW);
        digitalWrite(middleLaserPins[randomLaser2], LOW);

      }

      for (int i = 0; i < 10; i++) {
        digitalWrite(middleLaserPins[i], HIGH); // Turn on lasers
      }

      delay(500);

      for (int i = 0; i < 10; i++) {
        digitalWrite(middleLaserPins[i], LOW); // Turn off lasers
      }

      //flash all middle on beat
      //2:40-1:53 flash all lasers same time  


      //Repeat entire section 3 times..
      for (int i = 0; i < 3; i++) {

        for (int i = 0; i < 7; i++) {

          for (int i = 0; i < 6; i++) {
            digitalWrite(middleLaserPins[i], HIGH); // Turn on lasers
          }
      
          delay(230);  // Keep it on for 45 milliseconds

          for (int i = 0; i < 6; i++) {
            digitalWrite(middleLaserPins[i], LOW); // Turn on lasers
          }
          delay(230);

        }

        //snare drum? 1 time flash
        for (int i = 6; i < 10; i++) {
            digitalWrite(middleLaserPins[i], HIGH); // Turn on lasers
        }
      
        delay(200);  // Keep it on for 45 milliseconds

        for (int i = 6; i < 10; i++) {
          digitalWrite(middleLaserPins[i], LOW); // off on lasers
        }

        delay(200);  // Keep it on for 45 milliseconds

      }
      




      delay(2300);


  }

