// *******************************************************************************************************************************
// START userdefined data
// *******************************************************************************************************************************
#include <Arduino.h>
// Turn logging on/off - turn read logfile on/off, turn delete logfile on/off ---> default is false for all 3, otherwise it can cause battery drainage.
const bool  logging = false;
const bool  readLogfile = false;
const bool  deleteLogfile = false;
String readString; // do not change this variable

// Select DHT type on the module - supported are DHT11, DHT12, DHT22
//#define DHT_TYPE DHT11
#define DHT_TYPE DHT12
//#define DHT_TYPE DHT22

// It is a really good thing to calibrate each unit for soil, first note the number when unit is on the table, the soil number is for zero humidity. Then place the unit up to the electronics into a glass of water, the number now is the 100% humidity.
// By doing this you will get the same readout for each unit. Replace the value below for the dry condition, and the 100% humid condition, and you are done.

// Soil defaults - change them to your calibration data
int soil_min = 1535;
int soil_max = 3300;
bool calibrate_soil = false;

// Salt/Fertilizer recommandation break points. You can change these according to your own calibration measurements.
int fertil_needed = 200;
int fertil_low = 201;
int fertil_opt = 251;
int fertil_high = 351;

// Give the sensor a plant name, change to true, upload sketch and then revert to false
const bool update_plant_name = false;
String plant_name = "My_Test";


// define your SSID's, and remember to fill out variable ssidArrNo with the number of your SSID's
String ssidArr[] = {"Enterprise-pro", "Enterprise_EXT", "Enterprise_EXTN", "Enterprise" };
int ssidArrNo = 4;

const char* ssid = ""; // no need to fill in
const char* password = "password";
const char* ntpServer = "pool.ntp.org";

// Off-sets for time, and summertime. each hour is 3.600 seconds.
const long  gmtOffset_sec = 3600;

// Device configuration and name setting
const String device_name = "Tgrow_HIGrow"; // Can be changed, but not necessary, as it will give no added value.

#define uS_TO_S_FACTOR 1000000ULL //Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  3600       //Time ESP32 will go to sleep (in seconds)

const char broker[] = "192.168.1.64";
int        port     = 1883;
const char mqttuser[] = ""; //add eventual mqtt username
const char mqttpass[] = ""; //add eventual mqtt password

// *******************************************************************************************************************************
// END userdefined data
// *******************************************************************************************************************************
