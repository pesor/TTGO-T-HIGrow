#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <DHT.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <Esp.h>
#include <time.h>
#include <TimeLib.h>

#include "driver/adc.h"
#include <esp_wifi.h>
#include <esp_bt.h>
#include "user-variables.h"
#include <18B20_class.h>
#include <Adafruit_BME280.h>

// Logfile on SPIFFS
#include "SPIFFS.h"

//           rel = "2.0;    // Implemented MAC id as unique identifier for the device, at same time device_name is frozen to Tgrow_HIGrow.
//           rel = "2.0.1"; // Implemented "_" + name index for sensor icon. Corrected missing leading zero in HEX address.
//           rel = "2.0.2"; // Implemented automatic search for feaseable WIFI SSID, and connect to this.
//           rel = "3.0.0"; // Implemented Home-Assistant MQTT Autodiscover.
//           rel = "3.0.1"; // Implemented Home-Assistant MQTT Autodiscover, Salt calibration and advice included.
//           rel = "3.0.2"; // DST switch over now works
//           rel = "3.0.3"; // Small error corrections
//           rel = "3.0.4"; // Adapting to HACS frontend card: Battery State Card
//           rel = "3.0.5"; // Implemented name of plant saved to SPIFFS
//           rel = "3.0.6"; // DST calculation was way wrong, corrected now.
//           rel = "4.0.0"; // Changed from Arduino EDI to VS Code - PlatformIO
//           rel = "4.0.1"; // Error correction in connect network
//           rel = "4.0.2"; // Organising subroutines, and functional code snippets.
//           rel = "4.0.3"; // Adding battery charged date, and days since last charge
//           rel = "4.0.4"; // Adding battery charged date, and days since last charge, added to SPIFFS so that data do not dissapear at reboot.
//           rel = "4.0.5"; // Merged change from @reenari, and corrected counter days since last change
//           rel = "4.0.6"; // Corrected counter days !!! AGAIN !!!
//           rel = "4.0.7"; // The plant name is now used as hostname, so it is more visible in your router
//           rel = "4.1.0"; // Possibility to add the external 18B20 temperature sensor
//           rel = "4.2.0"; // BME280 sensor implemented
//           rel = "4.2.2"; // For the Greenhouse auto watering, the plantValveNo have been introduced. (Greenhouse auto watering is in development)
//           rel = "4.2.3"; // Removed the battery day counter - for good, use BeardedTingers solution if you need it.
//           rel = "4.3.1"; // Finally the days since last charging works correctly.
const String rel = "4.3.2"; // Corrected an error in DST.

// mqtt constants
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Date calculator
unsigned long epochTime;
String battChargeEpoc;
unsigned long epochChargeTime;
float battChargeDateDivider = 86400;
float daysOnBattery;

// Reboot counters
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int sleep5no = 0;

//Sensor bools
bool bme_found = false;

//json construct setup
struct Config
{
  String date;
  String time;
  int bootno;
  int sleep5no;
  float lux;
  float temp;
  float humid;
  float soil;
  float soilTemp;
  float salt;
  String saltadvice;
  float bat;
  String batcharge;
  String batchargeDate;
  float daysOnBattery;
  float batvolt;
  float batvoltage;
  float pressure;
  String rel;
};
Config config;

const int led = 13;

#define I2C_SDA 25
#define I2C_SCL 26
#define DHT_PIN 16
#define BAT_ADC 33
#define SALT_PIN 34
#define SOIL_PIN 32
#define BOOT_PIN 0
#define POWER_CTRL 4
#define USER_BUTTON 35
#define DS18B20_PIN 21

BH1750 lightMeter(0x23); //0x23
Adafruit_BME280 bmp;     //0x77

DHT dht(DHT_PIN, DHT_TYPE);
DS18B20 temp18B20(DS18B20_PIN);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String formattedDate;
String dayStamp;
String timeStamp1;

// Start Subroutines

#include <file-management.h>
#include <go-to-deep-sleep.h>
#include <get-string-value.h>
#include <read-sensors.h>
#include <save-configuration.h>
#include <connect-to-network.h>
#include <read-batt-info.h>
#include <floatConv.h>

void setup()
{
  Serial.begin(115200);
  Serial.println("Void Setup");

#include <module-parameter-management.h>

  // Start WiFi and update time
  connectToNetwork();
  Serial.println(" ");
  Serial.println("Connected to network");
  if (logging)
  {
    writeFile(SPIFFS, "/error.log", "Connected to network \n");
  }

  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.localIP());
  //  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //  timeClient.setTimeOffset(7200);

  timeClient.setTimeOffset(gmtOffset_sec);
  while (!timeClient.update())
  {
    timeClient.forceUpdate();
  }

#include <time-management.h>
  //#include <battChargeDays.h>
  if (dht_found)
  {
    dht.begin();
  }
  else
  {
    Serial.println(F("Could not find a valid DHT sensor, check if there is one present on board!"));
  }

  //! Sensor power control pin , use deteced must set high
  pinMode(POWER_CTRL, OUTPUT);
  digitalWrite(POWER_CTRL, 1);
  delay(1000);

  bool wireOk = Wire.begin(I2C_SDA, I2C_SCL); // wire can not be initialized at beginng, the bus is busy
  if (wireOk)
  {
    Serial.println(F("Wire ok"));
    if (logging)
    {
      writeFile(SPIFFS, "/error.log", "Wire Begin OK! \n");
    }
  }
  else
  {
    Serial.println(F("Wire NOK"));
  }

  if (!bmp.begin())
  {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    bme_found = false;
  }
  else
  {
    bme_found = true;
  }

  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
  {
    Serial.println(F("BH1750 Advanced begin"));
  }
  else
  {
    Serial.println(F("Error initialising BH1750"));
  }

  float luxRead = lightMeter.readLightLevel(); // 1st read seems to return 0 always
  Serial.print("lux ");
  Serial.println(luxRead);
  delay(2000);

  if (dht_found)
  {
    float t12 = dht.readTemperature(); // Read temperature as Fahrenheit then dht.readTemperature(true)
    config.temp = t12;
    delay(2000);
    float h12 = dht.readHumidity();
    config.humid = h12;
  }

  if (bme_found)
  {
    float bme_temp = bmp.readTemperature();
    config.temp = bme_temp;

    float bme_humid = bmp.readHumidity();
    config.humid = bme_humid;

    float bme_pressure = (bmp.readPressure() / 100.0F);
    config.pressure = bme_pressure;
  }

  uint16_t soil = readSoil();
  config.soil = soil;
  float soilTemp = readSoilTemp();
  config.soilTemp = soilTemp;

  uint32_t salt = readSalt();
  config.salt = salt;
  String advice;
  if (salt < 201)
  {
    advice = "needed";
  }
  else if (salt < 251)
  {
    advice = "low";
  }
  else if (salt < 351)
  {
    advice = "optimal";
  }
  else if (salt > 350)
  {
    advice = "too high";
  }
  Serial.println(advice);
  config.saltadvice = advice;

  // Battery status, and charging status and days.
  float bat = readBattery();
  config.bat = bat;
  config.batcharge = "";
  Serial.println("Battery level");
  Serial.println(bat);
  if (bat > 130)
  {
    config.batcharge = "charging";
    SPIFFS.remove("/batinfo.conf");
    epochChargeTime = timeClient.getEpochTime();
    battChargeEpoc = String(epochChargeTime) + ":" + String(dayStamp);
    const char *batinfo_write = battChargeEpoc.c_str();
    writeFile(SPIFFS, "/batinfo.conf", batinfo_write);
    Serial.println("dayStamp");
    Serial.println(dayStamp);
    config.batchargeDate = dayStamp;
  }

  Serial.println("Charge Epoc");
  Serial.println(battChargeEpoc);
  unsigned long epochTime = timeClient.getEpochTime();
  Serial.println("Test Epoc");
  Serial.println(epochTime);
  epochChargeTime = battChargeEpoc.toInt();
  Serial.println("first calculation");
  Serial.println(epochTime - epochChargeTime);
  float epochTimeFl = float(epochTime);
  float epochChargeTimeFl = float(epochChargeTime); 
  

  daysOnBattery = (epochTimeFl - epochChargeTimeFl) / battChargeDateDivider;
  daysOnBattery = truncate(daysOnBattery, 1);
  config.daysOnBattery = daysOnBattery;

  if (bat > 100)
  {
    config.bat = 100;
  }

  config.bootno = bootCount;

  luxRead = lightMeter.readLightLevel();
  Serial.print("lux ");
  Serial.println(luxRead);
  config.lux = luxRead;
  config.rel = rel;

  // Create JSON file
  Serial.println(F("Creating JSON document..."));
  if (logging)
  {
    writeFile(SPIFFS, "/error.log", "Creating JSON document...! \n");
  }
  saveConfiguration(config);

  // Go to sleep
  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Go to sleep now
  delay(1000);
  goToDeepSleep();
}

void loop()
{
}
