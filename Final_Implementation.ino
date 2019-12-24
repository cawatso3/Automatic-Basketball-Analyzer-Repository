#include <SensorLDR.h>
#include <Wire.h>
#include "SevSeg.h"
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#include "Arduino.h"
#include <StackArray.h>




//---------------------Session Timer--------------------------------------//
//session timer

unsigned long time;
unsigned long currentMillis;


const int ledPin =  LED_BUILTIN;// the number of the LED pin

int ledState = LOW;             // ledState used to set the LED

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated

// constants won't change:
const long interval = 1000;           // interval at which to blink (milliseconds)

//---------------------Shot Analyzer Variables--------------------------------------//
unsigned int count = 0;
unsigned int maxTimer = 5;
unsigned char TSTART = 0;
//unsigned int maxCount = 0;
unsigned long timerCount = 0;

unsigned char stopCountFlag = 0;
unsigned char waitFlag = 0;
unsigned char hoopWaitFlag = 0;

//unsigned char shotmade = 0;
unsigned char shotMadeFlag = 0;
unsigned int hoopentrycount = 0;

unsigned char hoopEntryFlag = 0;
unsigned char netMovementFlag = 0;

unsigned char bvFlag = 0;
unsigned char hvFlag = 0;

const long maxCount = 5000; //Maximum interval of 1 shot duration

int shotScore = 0;


//-------------------NetMovement/PhotoResistor---------------------------------------//
//PhotoResistor
SensorLDR sensor;

//-----------------------HoopEntry/BreakBeam------------------------------------//

#define SENSORPIN 4
int sensorState = 0, lastState = 0;       // variable for reading the pushbutton status


//----------------------Hoop Entry FSM-------------------------------------//


enum HoopStates {HESM_START, NoHoopEntry, HoopEntry, ShotMade} HoopState;

//void HoopEntry_Tick();

//----------------------Backboard Vibration-------------------------------------//
Adafruit_MMA8451 mma = Adafruit_MMA8451();


int bvTotal = 0;
long bvScore = 0;
//----------------------Hoop Vibration-------------------------------------//
//Adafruit_MMA8451 mma2 = Adafruit_MMA8451();

//----------------------Shot Score/Seven Segment-------------------------------------//
SevSeg sevseg;

unsigned long displayTimerCount = 0;

//-----------------------------------------------------------//

//Store shots in stack
// create a stack of numbers.
StackArray <int> stack;

int storeShotInterval = 500;
unsigned long storeShotMillis = 0;


void setup() {
  Serial.begin(9600);

  //-----------------------------------------------------------//

  //--------------------HoopEntry/Break Beam---------------------------------------//

  // initialize the sensor pin as an input:
  pinMode(SENSORPIN, INPUT);
  digitalWrite(SENSORPIN, HIGH); // turn on the pullup

  //-----------------------NetMovement/PhotoResistor------------------------------------//
  sensor.attach(A5);
  sensor.turnOn();

  //-----------------------BackboardVibration/ Accelerometer #1------------------------------------//
  if (! mma.begin()) {
    Serial.println("Couldnt start");
    while (1);
  }
  mma.setRange(MMA8451_RANGE_2_G);

  Serial.print("Range = "); Serial.print(2 << mma.getRange());
  Serial.println("G");


  //---------------------ShotScore/SevenSeg--------------------------------------//
  byte numDigits = 1;
  byte digitPins[] = {};
  byte segmentPins[] = {6, 7, 8, 9, 10, 11, 12, 13};
  bool resistorsOnSegments = true;

  byte hardwareConfig = COMMON_CATHODE;
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments);
  sevseg.setBrightness(90);


// set the printer of the stack.
stack.setPrinter (Serial);

}
























//---------------------Loop--------------------------------------//




void loop() {

  hoopEntry();
  HoopEntry_Tick();
 netMovement();
  backboardVibration();
  analyzeShot();
 displayScore();


  //-----------------Global Timer------------------------------------------//
  // check to see if it's time to blink the LED; that is, if the difference
  // between the current time and last time you blinked the LED is bigger than
  // the interval at which you want to blink the LED.
  currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    //-----------------Global Timer------------------------------------------//
    digitalWrite(ledPin, ledState);
    //Serial.print("Total Program Time: ");
    time = millis();
    //Serial.println(time/1000); //prints time since program started



  

    //    //-----------------Backboard Vibration Function Call---------------------------------//
    // backboardVibration();




    //-----------------Monitor Actions on Hoop------------------------------------------//




    //-----------------------------------------------------------//

    if (TSTART == 1) {    //TSTART is the global timer for the shot analyzer block, when == 1 the system begins counting
      //      if(waitFlag == 0){
      //        stopCountFlag = 0;
      //      }

      if (stopCountFlag == 0) {
        count++;
        //        Serial.println("count: ");
        //      Serial.println(count);

        if (count >= maxTimer) {    //When count == max seconds.


          if (waitFlag == 0) { //Flag to control if a shot was made to tell the timer to wait until shot score is analyzed
            stopCountFlag = 1;
            count = 0;
            TSTART = 0;
          }

        }
      }
    }

    

  }

  if (currentMillis - storeShotMillis >= storeShotInterval) {
    storeShotMillis = currentMillis;

    storeShot();
  }

  

}










//-----------------------NetMovement Working------------------------------------//

void netMovement() {
  int light = sensor.getLight();
  // Serial.println(light);

  if (light >= 120) {
    //    Serial.println("NetMovement = Valid: " );
    //    Serial.println(light);


    netMovementFlag = 1;

    //delay(500);
  }

  //    else {
  //      Serial.println("NetMovement = inValid: ");
  //      Serial.println(light);
  //      netMovementFlag = 0;
  //
  //    }
}

//-----------------------------------------------------------//



//-----------------------Hoop Entry------------------------------------//
void hoopEntry() {
  // read the state of the pushbutton value:
  sensorState = digitalRead(SENSORPIN);

  // check if the sensor beam is broken
  // if it is, the sensorState is LOW:


  if (sensorState && !lastState) {
    //Serial.println("Unbroken");
    hoopEntryFlag = 0;
    //    Serial.println("HoopEntryFlag: ");
    //    Serial.println(hoopEntryFlag);

  }
  else if (!sensorState && lastState) {
    //    if (TSTART == 0) {
    //      TSTART = 1;
    //    }
    hoopEntryFlag = 1;
    hoopWaitFlag = 1;
    //    Serial.println("HoopEntryFlag:Valid ");
    //    Serial.println(hoopEntryFlag);

  }
  lastState = sensorState;
}

//-----------------------------------------------------------//


//-----------------------Backboard Vibration------------------------------------//

void backboardVibration() {
  // Read the 'raw' data in 14-bit counts
  mma.read();

  /* Get a new sensor event */
  sensors_event_t event;
  mma.getEvent(&event);
  if (waitFlag == 0) {
    //if (abs(event.acceleration.x) >= 1.30 || abs(event.acceleration.y) >= 1.80) {

    //Max Vibration = lowest score
    if (abs(event.acceleration.x) >= 2.75 || abs(event.acceleration.y) >= 16  || abs(event.acceleration.z) >= 4.50) {
      bvScore = 1;
    }

    //Moderate Vibration = median score
    else if (abs(event.acceleration.x) < 2.75 && abs(event.acceleration.x) >= 0.50 || abs(event.acceleration.y) < 16 && abs(event.acceleration.y) >= 10.15 || abs(event.acceleration.z) < 4.50 && abs(event.acceleration.z) >= 1.20) {
      bvScore = 3;
    }


    //Minimal Vibration = Highest Score
    else if (abs(event.acceleration.x) < 0.50 || abs(event.acceleration.y) < 10.15 || abs(event.acceleration.z) < 1.20 ) {
      bvScore = 5;
    }

//    
//         Serial.println("bvscore: ");
//           Serial.println(bvScore);

//
//         Serial.println("x ");
//           Serial.println(event.acceleration.x);
//
//           
//         Serial.println("y");
//           Serial.println(event.acceleration.y);
//
//           
//         Serial.println("z");
//           Serial.println(event.acceleration.z);

//    delay(1000);
    



  }
}


//-----------------------------------------------------------//


//-----------------------Analyze Shot------------------------------------//
void analyzeShot() {
  if (waitFlag == 1) {
    shotScore = bvScore;
    //    Serial.println("ShotScore: ");
    //    Serial.println(shotScore);
    //



  }
}

//-----------------------------------------------------------//



//----------------------Display Score------------------------------------//
void displayScore() {

  if (waitFlag == 1) {
    sevseg.setNumber(bvScore);

    sevseg.refreshDisplay();
  }


  //waitFlag = 0;

}


//-----------------------------------------------------------//

void storeShot(){
   
    if (waitFlag == 1) {
   
   // print the values of the numbers.
  Serial.print ("shot score: "); Serial.println (bvScore);
  

  // push the numbers to the stack.
  stack.push (bvScore);
 
  // pop the numbers from the stack.
  bvScore = stack.pop ();
 waitFlag = 0; 
    }   
}
















//-----------------------------------------------------------//
void HoopEntry_Tick()
{
  switch (HoopState)
  {


    case HESM_START:


//      Serial.println("HoopState 0: ");
//      Serial.println(HoopState);
      HoopState = NoHoopEntry;
      break;

    case NoHoopEntry:
      if (waitFlag == 1) {
       // Serial.println("waiting... ");

      }

      else {

//        Serial.println("HoopState no hoop entry: ");
//        Serial.println(HoopState);
        //shotMadeFlag = 0;

        if (hoopEntryFlag == 0) { //Hoop Entry = 0
          if (netMovementFlag == 1) {
            netMovementFlag = 0;
          }

          HoopState = NoHoopEntry;
        }
        else if (hoopWaitFlag == 1) { //Hoop Entry = 1
          if (netMovementFlag == 1) {
            netMovementFlag = 0;
          }

          if (TSTART == 0) {
            TSTART = 1;
          }

          HoopState = HoopEntry;

        }
      }
      break;

    case HoopEntry:



//      Serial.println("Hoop state hoop entry: ");
//      Serial.println(HoopState);
//      Serial.println(stopCountFlag);
      //reset hoop entry flag for next shot attempt

      if (netMovementFlag == 0 && stopCountFlag == 0) {

        HoopState = HoopEntry; //Stay in hoop entry state to check for net movement until timer hits the max


      }

      else if (netMovementFlag == 0 && stopCountFlag == 1) {  //Stop counting, reset TSTART
        stopCountFlag = 0;
        TSTART = 0;
        HoopState = NoHoopEntry;
      }

      else if (netMovementFlag == 1 && stopCountFlag == 0) { //netmovement = 1 VALID SHOT
        waitFlag = 1;
        shotMadeFlag = 1;
        HoopState = ShotMade;  //Shot is valid as Hoop Entry And Net movement were detected within time range, go to next state

      }
      else if (netMovementFlag == 1 && stopCountFlag == 1) {  //max time reached
        stopCountFlag = 0; //Reset
        netMovementFlag = 0;
        TSTART = 0;
        HoopState = NoHoopEntry;

      }

      break;

    case ShotMade:

      //      Serial.println("HoopState shot made: ");
      //      Serial.println(HoopState);

      waitFlag = 1;

      shotMadeFlag = 1;


      HoopState = NoHoopEntry;


      break;

    default:
      HoopState = HESM_START;
      break;
  }
}

//
// //-----------------------------------------------------------//
