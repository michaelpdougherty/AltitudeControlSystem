#define TRUE  1
#define FALSE 0

#define RELEASE      1
#define CLOSED       2
#define FLIGHT       3

#define GROUND       0
#define ASCENT       1
#define VENTING      2
#define FLOATING     3
#define DESCENT      4

#define RELEASE_POS   35
#define OPEN_POS      35
#define CLOSED_POS    98

#define VENT_ALT      10000     // Meters (21000 ~ 68,900 feet)
#define FLOAT_TIME    1         // Minutes
#define MAXFLOATALT   27500     // Meters (27500 ~ 90,000 feet)

#define NN_dAdt 40

#define SIZE 255                   // Size of filename char arrays

// file types
#define DATA 0
#define ERR 1
#define STATUS 2



// VENTING CALCULATION //////////////////////////////////////////

#define TIME_IN_RANGE 5000      // time to take venting calculation (ms)
#define ASCENT_RATE_RANGE 0.75  // ascent rate range necessary for venting (dA/dt)
#define CONFIDENCE_LEVEL 0.95   // minimum percentage of measurements that must be within ASCENT_RATE_RANGE in a given TIME_IN_RANGE

long int floatStartTime=0;
long int timeOffset=0;
float currentActuatorExtension=50;

// declare venting variables
float countInRange = 0;     // number of times measurement was within ASCENT_RATE_RANGE
float countOutRange = 0;    // number of times measurement was outside ASCENT_RATE_RANGE
float timeEnteredRange = 0; // time (ms) at which last ascent rate timer began
float timeInRange = 0;      // time (ms) since ascent rate measurement has initiated timer, calculated from timeEnteredRange

/////////////////////////////////////////////////////////////////



// FILENAME CONSTRUCTION ////////////////////////////////////////

// Files for reading SD card
File root, entry;

// Flight folder param
String FOLDER_NAME = "FLIGHT";

// Data file params
String DATA_BODY = "DATA";
String DATA_EXTENSION = ".DAT";

// Error file params
String ERROR_BODY = "ERROR";
String ERROR_EXTENSION = ".TXT";

// Status file params
String STATUS_BODY = "STATUS";
String STATUS_EXTENSION = ".TXT";

// default vars for flight number as integer and string
unsigned int FLIGHT_NUMBER_INT = 0;
String FLIGHT_NUMBER_STRING = "000";

// empty char arrays for SD functions (doesn't allow Strings)
char FILENAME_DATA[SIZE] = {};
char FILENAME_ERROR[SIZE] = {};
char FILENAME_STATUS[SIZE] = {};
char FILENAME_FOLDER[SIZE] = {};

// String var to be changed and compared against files on SD card
String checkFilename = FOLDER_NAME + FLIGHT_NUMBER_STRING;

// function to read SD card and update flight number integer
void getFilenames();

// converts flight number integer to string
void updateFlightNumberString();

/////////////////////////////////////////////////////////////////

// globally manipulable data string
String dataString = {};

// globally accessible system time
long int currentTime = 0;

// declare baud rate
const unsigned int BAUD_RATE = 57600;

// declare headers
String HEADERS = "Time (ms), Time (s), Time (H:M:S), Switch State, Switch State (0-2), Program State, Program State (0-4), Actuator Extension, Pressure (mbar), Altitude (m), Ascent Rate (m/s), Temp. (deg C), Teensy Temp. (deg C)";

int programState;           // current ACS stage as an integer 0-4 (GROUND, ASCENT, VENTING, FLOATING, and DESCENT)

// SD Card
File LOG;
File missionFile;
void initSD();
long int lastLogTime;


// OLED Setup
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif


// Actuator
void SetStrokePerc(float strokePercentage);
int actuator_pin = 4;       //actuator Pin
Servo actuator;             //create servo objects to control the linear actuator


// three way switch setup
int switch1 = 1;            //switch pin that starts Flight sequence (red)
int switch2 = 0;            //middle switch pin that moves actuator into closed position (blue)
int switch3 = 2;            //switch pin that runs Testing sequence (black)


// button setup
int buttonPin = 3;    //button pin
boolean buttonPressed;


// pressure sensor setup
#define ADDRESS 0x77 // Address of Far Horizons - Pressure Sensor MS5607 REV B
void pressure_setup();
float get_pressure(float *Tem);

// at most 20 triggers
char *triggerStateNames[20];
float triggerAmin[20];
float triggerAmax[20];
float triggerDAdtmin[20];
float triggerDAdtmax[20];
int triggerTimemax[20];
char *triggerDestStateNames[20];
