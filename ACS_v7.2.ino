#include <math.h>             // math .. Pressure Sensor needs this
//#include <String.h>

#include <Servo.h>            //Linear Actuator
#include <Wire.h>             //I2C library fort OLED and Pressure Sensor
#include <SPI.h>              //SPI library for SD card
#include <SD.h>               //SD    ********MUST BE VERSION 1.2.2 TO WORK WITH TEENSY********

#include <Adafruit_GFX.h>     //OLED libary
#include <Adafruit_SSD1306.h> //OLED library
#include <EEPROM.h>

#include "Globals.h"


void setup() {

  char lineBuf[150];
  
  // set control pins for 3-way switch to input
  pinMode(switch1, INPUT_PULLUP);
  pinMode(switch2, INPUT_PULLUP);
  pinMode(switch3, INPUT_PULLUP);
  
  // and initialize the button
  pinMode(buttonPin, INPUT_PULLUP);
  buttonPressed=FALSE;

  // OLED setup
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);    //Initialize OLED
  display.display();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Serial port for debugging
  Serial.begin(BAUD_RATE);

  // attach actuator
  actuator.attach(actuator_pin);

  // set up pressure sensor
  pressure_setup();
  
  // initialize SD card
  initSD();

  // create filenames
  getFilenames();

  // create flight folder
  SD.mkdir(FILENAME_FOLDER);
  logData("Created folder " + String(FILENAME_FOLDER), STATUS);

  // get mission profile
//  getMissionProfile();
 
  if (switchPos()==FLIGHT) {
    logData("System started in FLIGHT mode", STATUS);
    // here we will read the last line of the log file to get:
    //  program state
    //  currenttime (timeOffset)
     
    // open logfile
    if ((LOG = SD.open(FILENAME_DATA, FILE_READ))) { 
      
      while (LOG.available()) {
        LOG.readBytesUntil('\n',lineBuf,150);    
      }
      Serial.print("last line: ");
      Serial.println(lineBuf);
      // we should have the last line now
      sscanf(lineBuf, "%ld %*d %d %f %ld",&timeOffset, &programState, &currentActuatorExtension, &floatStartTime);
      Serial.println(timeOffset);
      Serial.println(programState);
      Serial.println(currentActuatorExtension);
      Serial.println(floatStartTime);
  
      SetStrokePerc(currentActuatorExtension);

      // close log file
      LOG.close();
    }
  }
   
  // open data file
  if (!(LOG = SD.open(FILENAME_DATA, FILE_WRITE))) {
    logData("Data file failed to open", ERR);
  } else {
    lastLogTime=-9999;
  
    // print headers to data file
    logData(HEADERS, DATA);
  }
  
}

void loop() {
  String switchStateString, programStateString;
  int switchState,i;
  float currentPressure,currentTemp,currentTeensyTemp,currentAltitude,currentDAdt;
  float pressureArray[11];
  // get time
  currentTime=millis()+timeOffset;
  
  // process switch and button
  switchState = switchPos();
  if (digitalRead(buttonPin) == HIGH) buttonPressed=TRUE; 

  /*
  // get sensor values
  for (i=0;i<11;i++) {
     pressureArray[i]=get_pressure(&currentTemp);
  }
  currentPressure=median(pressureArray,11)
  */
    
  currentPressure=get_pressure(&currentTemp);
  currentTeensyTemp=getTeensyTemp();

  // derive altitude and derivative  
  currentAltitude=altitudeFromPressure(currentPressure);
  currentDAdt=dAdt_fit2(currentAltitude,currentTime);

  display.clearDisplay();
  display.setCursor(0,20);

  switch (switchState) {
    case(RELEASE):
        switchStateString = "RELEASE";
        display.setCursor(0,0);
        display.println("RELEASE");
        if (buttonPressed) {
            display.println("Button pressed");
            logData("Button pressed in RELEASE mode, releasing", STATUS);
            delay(100);
            buttonPressed=FALSE;
            SetStrokePerc(RELEASE_POS);
//            currentActuatorExtension=RELEASE_POS;
        }
        programState=GROUND;
        logData("Program state switched to GROUND", STATUS);
        display.display();
        break;
    case(CLOSED):
      switchStateString = "CLOSED";
        display.setCursor(0,0);
        display.println("CLOSED");
        display.println(currentPressure);
        display.display();
        SetStrokePerc(CLOSED_POS);
//        currentActuatorExtension=CLOSED_POS;
        programState=GROUND;
        break;
    case(FLIGHT):
        switchStateString = "FLIGHT";
        display.setCursor(0,0);
        display.print("FLIGHT - ");
        // print program state from number
        switch (programState) {
          case (GROUND):
            programStateString = "GROUND";
            break;
          case (ASCENT):
            programStateString = "ASCENT";
            break;
          case (VENTING):
            programStateString = "VENTING";
            break;
          case (FLOATING):
            programStateString = "FLOATING";
            break;
          case (DESCENT):
            programStateString = "DESCENT";
            break;
          default:
            programStateString = "Error";
            break;
        }
        display.println(programStateString);

        
        // print headers
        display.println("Pres  Alt  Vel");
        display.println("mbar  m    m/s");
        display.print(currentPressure,1);
        display.print("  ");
        display.print(currentAltitude,0);
        display.print("  ");
        display.println(currentDAdt);
        
        switch (programState) {
          case (GROUND):
              SetStrokePerc(CLOSED_POS);            // this may take a lot of energy if the actuator can't quite get there
//              currentActuatorExtension=CLOSED_POS;
              // set program state to ascent if altitude is greater than 2000 meters in case start is forgotten
              if (currentAltitude>2000) {
                logData("Reached 2,000 m, switched to ASCENT", STATUS);
                logData("Someone forgot to initiate the flight. Failsafe activated at 2,000 m", ERR);
                programState=ASCENT;
              }
              if (!buttonPressed) display.println("Press to start");
              if (buttonPressed) {
                 logData("Button pressed in FLIGHT mode, switched to ASCENT", STATUS);
                 display.println("Flight Initiated");
                 display.display();
                 delay(1000);
                 buttonPressed=FALSE;
                 programState=ASCENT;
                 
              }
              break;
          case (ASCENT):
              if (currentAltitude>VENT_ALT) {     // this is in meters
                logData("Reached " + String(VENT_ALT) + " m venting altitude, switched to VENTING", STATUS);
                logData("Actuator set to OPEN position", STATUS);
                SetStrokePerc(OPEN_POS);
//                currentActuatorExtension=OPEN_POS;
                programState=VENTING;
              }
              break;
          case (VENTING):
              float confidenceLevel;
              float confidencePercentage;
              // Velocity is within desired range
              if (abs(currentDAdt) <= ASCENT_RATE_RANGE) {
                logData("Ascent rate is within desired range at " + String(currentDAdt) + " m/s at " + String(currentAltitude) + "m altitude", STATUS);
                // increment count
                countInRange++;

                // balloon has just entered range, begin timer
                if (timeInRange == 0) {
                  // update time entered
                  timeEnteredRange = currentTime;

                  display.println("Entered range");
                  display.display();

                  // set time in range above 0 to avoid an infinite loop
                  timeInRange = 0.1;       
                } else {
                  // balloon has been in range, update timer
                  timeInRange = currentTime - timeEnteredRange;
                }
              } else {
                // Velocity is outside desired range, increment count
                countOutRange++;
              }

              // balloon first entered within range TIME_IN_RANGE seconds ago
              if (timeInRange >= TIME_IN_RANGE) {
                // calculate confidence level
                confidenceLevel = countInRange / (countOutRange + countInRange);
                confidencePercentage = confidenceLevel * 100;
                if (confidenceLevel >= CONFIDENCE_LEVEL) {
                  SetStrokePerc(CLOSED_POS);
//                  currentActuatorExtension=CLOSED_POS;
                  logData("Reached desired confidence level, switched to FLOATING", STATUS);
                  logData("Altitude: " + String(currentAltitude) + " m", STATUS);
                  logData("Confidence: " + String(confidencePercentage) + "%", STATUS);
                  logData("Times within range: " + String(countInRange), STATUS);
                  logData("Times outside range: " + String(countOutRange), STATUS);
                  logData("Actuator set to CLOSED position", STATUS);
                  programState=FLOATING; 
                  floatStartTime=currentTime; 
                } else {
                  // confidence value too low, reset and try again
                  countInRange = 0;
                  countOutRange = 0;
                  timeInRange = 0;
                  timeEnteredRange = 0;
                  // log what happened
                  logData("Desired confidence level attempted but not achieved", STATUS);
                  logData("Altitude: " + String(currentAltitude) + " m", STATUS);
                  logData("Confidence: " + String(confidencePercentage) + "%", STATUS);
                  logData("Times within range: " + String(countInRange), STATUS);
                  logData("Times outside range: " + String(countOutRange), STATUS);
                }
              }
              break;
          case (FLOATING):
              if (currentTime-floatStartTime>FLOAT_TIME*60*1000) {
                logData("FLOATING period completed after " + String(FLOAT_TIME) + " minutes, switching to DESCENT", STATUS);
                
                logData("Actuator set to OPEN position", STATUS);
                SetStrokePerc(RELEASE_POS);
//                currentActuatorExtension=RELEASE_POS;
                programState=DESCENT;       
              }
              if (currentAltitude>MAXFLOATALT) {
                logData("Maximum floating altitude met at " + String(MAXFLOATALT) + " m, switching to DESCENT", STATUS);
                logData("Maximum floating altitude met at " + String(MAXFLOATALT) + " m, switched to DESCENT as failsafe", STATUS);
              }
              break;
          case (DESCENT):
              break;                  
        }
        display.display();
        break;
  }
  delay(300);
  buttonPressed=FALSE;

  // log the current state once a second
  if ((currentTime-lastLogTime)>1000) {
    // update data string
    dataString = String(currentTime) + "," + String(currentTime / 1000) + "," + formatTime() + "," + String(switchStateString) + "," + String(switchState) + "," + String(programState) + "," + String(programStateString) + "," + String(currentActuatorExtension) + "," + String(currentPressure) + "," + String(currentAltitude) + "," + String(currentDAdt) + "," + String(currentTemp) + "," + String(currentTeensyTemp);
    // log to file and serial
    logData(dataString, DATA);
    // update last log time
    lastLogTime=currentTime;
  } 
}
