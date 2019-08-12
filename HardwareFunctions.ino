//////////////////////////////////////////////////////////////////////////////////////
//                                    SD Card                                      ///
//////////////////////////////////////////////////////////////////////////////////////

void initSD() {
  logData("Initializing SD card...", STATUS);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(BUILTIN_SDCARD)) {
    logData("card failed or not present", STATUS);
    display.println("CARD FAILED OR NO");
    display.println("SD CARD");
  } else {
    logData("card initialized", STATUS);
    display.println("CARD INITIALIZED");
  }
  display.display();
  delay(2000);
}

void getFilenames() {
  // iterator
  unsigned int q;
  
  // open root
  root = SD.open("/");

  // iterate over files in root, checking against current flight number
  while (true) {
    entry = root.openNextFile();

    // no more files
    if (!entry) break;

    // check if folder matches current flight number
    if ( String(entry.name()).equals(checkFilename) ) {
      // folder exists, increment integer, string, and filename
      FLIGHT_NUMBER_INT++;
      updateFlightNumberString();
      checkFilename = FOLDER_NAME + FLIGHT_NUMBER_STRING;
    } 
  }

  // reached end of filesystem, transfer String to char array
  for (q = 0; q < SIZE; q++) {
    FILENAME_FOLDER[q] = checkFilename[q];
  }

  // repeat for data filename path
  checkFilename = String(FILENAME_FOLDER) + "/" + DATA_BODY + FLIGHT_NUMBER_STRING + DATA_EXTENSION;
  for (q = 0; q < SIZE; q++) {
    FILENAME_DATA[q] = checkFilename[q];
  }

  // error filename path
  checkFilename = String(FILENAME_FOLDER) + "/" + ERROR_BODY + FLIGHT_NUMBER_STRING + ERROR_EXTENSION;
  for (q = 0; q < SIZE; q++) {
    FILENAME_ERROR[q] = checkFilename[q];
  }

  // status filename path
  checkFilename = String(FILENAME_FOLDER) + "/" + STATUS_BODY + FLIGHT_NUMBER_STRING + STATUS_EXTENSION;
  for (q = 0; q < SIZE; q++) {
    FILENAME_STATUS[q] = checkFilename[q];
  }  
}

void updateFlightNumberString() {
  // determine number of digits in flight number
  if (FLIGHT_NUMBER_INT >= 100) {
    // flight number has 3 digits
    FLIGHT_NUMBER_STRING = String(FLIGHT_NUMBER_INT);
  } else if (FLIGHT_NUMBER_INT >= 10) {
    // flight number has 2 digits
    FLIGHT_NUMBER_STRING = "0" + String(FLIGHT_NUMBER_INT);
  } else {
    // flight number has 1 digit
    FLIGHT_NUMBER_STRING = "00" + String(FLIGHT_NUMBER_INT);
  }
}


///////////////////////////////////////////////////////////////////////////////////////
//                                    Actuator                                      ///
///////////////////////////////////////////////////////////////////////////////////////

void SetStrokePerc(float strokePercentage)
{
  if ( strokePercentage >= 1.0 && strokePercentage <= 99.0 )
  {
    int usec = 1000 + strokePercentage * ( 2000 - 1000 ) / 100.0 ;
    actuator.writeMicroseconds( usec );
  }
  currentActuatorExtension=strokePercentage;
}



///////////////////////////////////////////////////////////////////////////////////////
//                                    Pressure Sensor                               ///
///////////////////////////////////////////////////////////////////////////////////////
long getVal(int address, byte code);
void initial(uint8_t address);
uint32_t D1 = 0;
uint32_t D2 = 0;
float dT = 0;
int32_t TEMP = 0;
float OFF = 0;
float SENS = 0;
float P = 0;
float T2  = 0;
float OFF2  = 0;
float SENS2 = 0;
uint16_t C[7];

float Temperature, Pressure;

void pressure_setup() {
  
// Disable internal pullups, 10Kohms are on the breakout
 PORTC |= (1 << 4);
 PORTC |= (1 << 5);
  
  Wire.begin();
  //  Serial.begin(57600); //9600 changed 'cos of timing?
  delay(1000); //Give the open log a second to get in gear.
  initial(ADDRESS);
}

float get_pressure(float *Tem)
{
//count = count + 1;

  D1 = getVal(ADDRESS, 0x48); // Pressure raw
  D2 = getVal(ADDRESS, 0x58);// Temperature raw

  dT   = (float)D2 - ((uint32_t)C[5] * 256);
  OFF  = ((float)C[2] * 131072) + ((dT * C[4]) / 64);
  SENS = ((float)C[1] * 65536) + (dT * C[3] / 128);

  TEMP = (int64_t)dT * (int64_t)C[6] / 8388608 + 2000;

 if(TEMP < 2000) // if temperature lower than 20 Celsius
  {

    T2=pow(dT,2)/2147483648;
    OFF2=61*pow((TEMP-2000),2)/16;
    SENS2=2*pow((TEMP-2000),2);

   if(TEMP < -1500) // if temperature lower than -15 Celsius
    {
     OFF2=OFF2+15*pow((TEMP+1500),2);
     SENS2=SENS2+8*pow((TEMP+1500),2);
    }

    TEMP = TEMP - T2;
    OFF = OFF - OFF2;
    SENS = SENS - SENS2;
  }

  Temperature = (float)TEMP / 100;
  P  = (D1 * SENS / 2097152 - OFF) / 32768;
  Pressure = (float)P / 100;

//  Serial.print("$FHDAT");
//  Serial.print(",");
////  Serial.print(count);
//  Serial.print("\t");
//  Serial.print(Temperature);
//  Serial.print("\t");
//  Serial.println(Pressure);

  /* RESET THE CORECTION FACTORS */

    T2 = 0;
    OFF2 = 0;
    SENS2 = 0;
    
*Tem=Temperature;
return Pressure;
  //delay(1000);
}

long getVal(int address, byte code)
{
  unsigned long ret = 0;
  Wire.beginTransmission(address);
  Wire.write(code);
  Wire.endTransmission();
  delay(10);
  // start read sequence
  Wire.beginTransmission(address);
  Wire.write((byte) 0x00);
  Wire.endTransmission();
  Wire.beginTransmission(address);
  Wire.requestFrom(address, (int)3);
  if (Wire.available() >= 3)
  {
    ret = Wire.read() * (unsigned long)65536 + Wire.read() * (unsigned long)256 + Wire.read();
  }
  else {
    ret = -1;
  }
  Wire.endTransmission();
  return ret;
}

void initial(uint8_t address)
{
  //Serial.println();
  //Serial.println("PRESSURE SENSOR PROM COEFFICIENTS");

  Wire.beginTransmission(address);
  Wire.write(0x1E); // reset
  Wire.endTransmission();
  delay(10);


  for (int i=0; i<6  ; i++) {

    Wire.beginTransmission(address);
    Wire.write(0xA2 + (i * 2));
    Wire.endTransmission();

    Wire.beginTransmission(address);
    Wire.requestFrom(address, (uint8_t) 6);
    delay(1);
    if(Wire.available())
    {
       C[i+1] = Wire.read() << 8 | Wire.read();
    }
    else {
      Serial.println("Error reading PROM 1"); // error reading the PROM or communicating with the device
    }
    //Serial.println(C[i+1]);   // Prints out the coefficients.
  }
  //Serial.println();
}


//////////////////////////////////////////////////////////////////////////////////////////////
//                                    Switches                                             ///
//////////////////////////////////////////////////////////////////////////////////////////////
int switchPos() {
  if (digitalRead(switch1) == LOW) return RELEASE; 
  if (digitalRead(switch2) == LOW) return CLOSED;
  if (digitalRead(switch3) == LOW) return FLIGHT; 
}


//////////////////////////////////////////////////////////////////////////////////////////////
//                                    Teensy 3.5                                           ///
//////////////////////////////////////////////////////////////////////////////////////////////



float getTeensyTemp() {
  static bool initialized=false; // Could be done 'once' in setup() - or something like this perhaps
  if ( !initialized ) {
    initialized = true;
    analogReference(INTERNAL);
    analogReadResolution(16);
    analogReadAveraging(32);
  }
  int a;
  a = analogRead(70);   // k66 70
  return -0.00825*a +351.15; 


 }
