#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <DHT.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <ESP.h>

#include "driver/adc.h"
#include <esp_wifi.h>
#include <esp_bt.h>
#include "user-variables.h"

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
const String rel = "4.0.0"; // Changed from Arduino EDI to VS Code - PlatformIO

// mqtt constants
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Reboot counters
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int sleep5no = 0;


//json construct setup
struct Config {
  String date;
  String time;
  int bootno;
  int sleep5no;
  float lux;
  float temp;
  float humid;
  float soil;
  float salt;
  String saltadvice;
  float bat;
  String batcharge;
  float batvolt;
  float batvoltage;
  String rel;
};
Config config;

const int led = 13;

#define I2C_SDA             25
#define I2C_SCL             26
#define DHT_PIN             16
#define BAT_ADC             33
#define SALT_PIN            34
#define SOIL_PIN            32
#define BOOT_PIN            0
#define POWER_CTRL          4
#define USER_BUTTON         35

BH1750 lightMeter(0x23); //0x23
Adafruit_BME280 bmp;     //0x77 Adafruit_BME280 is technically not used, but if removed the BH1750 will not work - Any suggestions why, would be appriciated.

DHT dht(DHT_PIN, DHT_TYPE);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String formattedDate;
String dayStamp;
String timeStamp1;

bool bme_found = false;

void writeFile(fs::FS & fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}

void readFile(fs::FS & fs, const char * path) {
  //  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  //  Serial.println("- read from file:");
  while (file.available()) {
    delay(2);  //delay to allow byte to arrive in input buffer
    char c = file.read();
    readString += c;
  }
  //  Serial.println(readString);
  file.close();
}

void listDir(fs::FS & fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}


void goToDeepSleep()
{
  Serial.print("Going to sleep... ");
  Serial.print(TIME_TO_SLEEP);
  Serial.println(" seconds");
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Going to sleep for 3600 seconds \n");
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();

  // Configure the timer to wake us up!
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // Go to sleep! Zzzz
  esp_deep_sleep_start();
}
void goToDeepSleepFiveMinutes()
{
  Serial.print("Going to sleep... ");
  Serial.print("300");
  Serial.println(" sekunder");
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Going to sleep for 300 seconds \n");
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();

  adc_power_off();
  esp_wifi_stop();
  esp_bt_controller_disable();

  // Configure the timer to wake us up!
  ++sleep5no;
  esp_sleep_enable_timer_wakeup(300 * uS_TO_S_FACTOR);

  // Go to sleep! Zzzz
  esp_deep_sleep_start();
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

// READ Sensors
// I am not quite sure how to read and use this number. I know that when put in water wich a DH value of 26, it gives a high number, but what it is and how to use ??????
uint32_t readSalt()
{
  uint8_t samples = 120;
  uint32_t humi = 0;
  uint16_t array[120];

  for (int i = 0; i < samples; i++) {
    array[i] = analogRead(SALT_PIN);
    //    Serial.print("Read salt pin : ");

    //    Serial.println(array[i]);
    delay(2);
  }
  std::sort(array, array + samples);
  for (int i = 0; i < samples; i++) {
    if (i == 0 || i == samples - 1)continue;
    humi += array[i];
  }
  humi /= samples - 2;
  return humi;
}

uint16_t readSoil()
{
  Serial.println(soil_max);
  uint16_t soil = analogRead(SOIL_PIN);
  Serial.print("Soil before map: ");
  Serial.println(soil);
  return map(soil, soil_min, soil_max, 100, 0);
}

float readBattery()
{
  int vref = 1100;
  uint16_t volt = analogRead(BAT_ADC);
  Serial.print("Volt direct ");
  Serial.println(volt);
  config.batvolt = volt;
  float battery_voltage = ((float)volt / 4095.0) * 2.0 * 3.3 * (vref) / 1000;
  config.batvoltage = battery_voltage;
  Serial.print("Battery Voltage: ");
  Serial.println(battery_voltage);
  battery_voltage = battery_voltage * 100;
  return map(battery_voltage, 416, 290, 100, 0);
}

// Allocate a  JsonDocument
void saveConfiguration(const Config & config) {

  //  Serial.println(WiFi.macAddress());
  //  String stringMAC = WiFi.macAddress();
  //  stringMAC.replace(':', '_');

  byte mac[6];
  WiFi.macAddress(mac);

  //  String chipId = String(mac[0], HEX) + String(mac[1], HEX) + String(mac[2], HEX) + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
  String chipId = "";
  String HEXcheck = "";
  for (int i = 0; i <= 5; i++) {
    HEXcheck = String(mac[i], HEX);
    if (HEXcheck.length() == 1) {
      chipId = chipId + "0" + String(mac[i], HEX);
    } else {
      chipId = chipId + String(mac[i], HEX);
    }
  }
  Serial.println("chipId " + chipId);
  const String topicStr = device_name + "/" + chipId;
  const char* topic = topicStr.c_str();
  Serial.println(topic);
  Serial.println(ssid);
  StaticJsonDocument<1024> doc;
  // Set the values in the document
  // Device changes according to device placement
  JsonObject root = doc.to<JsonObject>();

  JsonObject plant = root.createNestedObject("plant");
  plant[device_name] = chipId;
  plant["sensorname"] = plant_name;
  plant["date"] = config.date;
  plant["time"] = config.time;
  plant["sleep5Count"] = config.sleep5no;
  plant["bootCount"] = config.bootno;
  plant["lux"] = config.lux;
  plant["temp"] = config.temp;
  plant["humid"] = config.humid;
  plant["soil"] = config.soil;
  plant["salt"] = config.salt;
  plant["saltadvice"] = config.saltadvice;
  plant["bat"] = config.bat;
  plant["batcharge"] = config.batcharge;
  plant["battvolt"] = config.batvolt;
  plant["battvoltage"] = config.batvoltage;
  plant["wifissid"] = WiFi.SSID();
  plant["rel"] = config.rel;

  // Send to mqtt
  char buffer[1024];
  serializeJson(doc, buffer);


  Serial.print("Sending message to topic: ");
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Sending message to topic: \n");
  }

  Serial.println(buffer);

  // Connect to mqtt broker
  Serial.print("Attempting to connect to the MQTT broker: ");
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Attempting to connect to the MQTT broker! \n");
  }

  Serial.println(broker);
  mqttClient.setServer(broker, 1883);

  if (!mqttClient.connect(broker, mqttuser, mqttpass)) {
    if (logging) {
      writeFile(SPIFFS, "/error.log", "MQTT connection failed! \n");
    }

    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.state());
    goToDeepSleepFiveMinutes();
  }

  if (logging) {
    writeFile(SPIFFS, "/error.log", "You're connected to the MQTT broker! \n");
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  bool retained = true;

  if (mqttClient.publish(topic, buffer, retained)) {
    Serial.println("Message published successfully");
  } else {
    Serial.println("Error in Message, not published");
    goToDeepSleepFiveMinutes();
  }
  Serial.println();
}

void connectToNetwork() {
  Serial.print("Size of SSID array ");
  Serial.println(ssidArrNo);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  bool breakLoop = false;
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Connecting to Network: \n");
  }
  for (int i = 0; i <= ssidArrNo; i++) {
    ssid = ssidArr[i].c_str();
    Serial.print("SSID name: ");
    Serial.print(ssidArr[i]);

    while ( WiFi.status() !=  WL_CONNECTED )
    {
      // wifi down, reconnect here
      WiFi.begin(ssid, password);
      int WLcount = 0;
      int UpCount = 0;
      while (WiFi.status() != WL_CONNECTED )
      {
        delay( 100 );
        Serial.printf(".");
        if (UpCount >= 60)  // just keep terminal from scrolling sideways
        {
          UpCount = 0;
          Serial.printf("\n");
        }
        ++UpCount;
        ++WLcount;
        if (WLcount > 200) {
          Serial.println("we should break");
          breakLoop = true;
          break;
        }
      }
      if (breakLoop) {
        breakLoop = false;
        break;
      }
    }
  }
  if (WiFi.status() !=  WL_CONNECTED) {
    goToDeepSleepFiveMinutes();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Void Setup");

  // Initiate SPIFFS and Mount file system
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    if (logging) {
      writeFile(SPIFFS, "/error.log", "An Error has occurred while mounting SPIFFS \n");
    }
    return;
  }
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Void Setup \n");
  }

  listDir(SPIFFS, "/", 0);

  if (logging) {
    writeFile(SPIFFS, "/error.log", "After listDir \n");
  }

  if (readLogfile) {
    // Now we start reading the files..
    readFile(SPIFFS, "/error.log");
    Serial.println("Here comes the logging info:");
    Serial.println(readString);
  }

  if (deleteLogfile) {
    SPIFFS.remove("/error.log");
  }

  // Calibrate Soil figures save to file, if calibrate_soil == true
  if (calibrate_soil) {
    SPIFFS.remove("/soil.conf");
    String soil_write_str = String(soil_min) + ":" + String(soil_max);
    const char* soil_write = soil_write_str.c_str();
    writeFile(SPIFFS, "/soil.conf", soil_write);
  } else {
    readFile(SPIFFS, "/soil.conf");
    Serial.println("Here comes the calibration info:");
    Serial.println(readString);
    String xval = getValue(readString, ':', 0);
    String yval = getValue(readString, ':', 1);

    soil_min = xval.toInt();
    soil_max = yval.toInt();
    readString = "";
  }

  if (update_plant_name) {
    SPIFFS.remove("/name.conf");
    String name_write_str = plant_name;
    const char* name_write = name_write_str.c_str();
    writeFile(SPIFFS, "/name.conf", name_write);
  } else {
    readFile(SPIFFS, "/name.conf");
    Serial.println("Here comes the name info:");
    Serial.println(readString);
    plant_name = readString;
    readString = "";
  }

  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Before Start WIFI \n");
  }

  // Start WiFi and update time
  connectToNetwork();
  Serial.println(" ");
  Serial.println("Connected to network");
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Connected to network \n");
  }

  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.localIP());
  //  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //  timeClient.setTimeOffset(7200);

  Wire.begin(I2C_SDA, I2C_SCL);
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Wire Begin OK! \n");
  }

  dht.begin();
  if (logging) {
    writeFile(SPIFFS, "/error.log", "DHT12 Begin OK! \n");
  }

  //! Sensor power control pin , use deteced must set high
  pinMode(POWER_CTRL, OUTPUT);
  digitalWrite(POWER_CTRL, 1);
  delay(1000);

  if (!bmp.begin()) {
    Serial.println(F("This check must be done, otherwise the BH1750 does not initiate!!!!?????"));
    bme_found = false;
  } else {
    bme_found = true;
  }

  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("BH1750 Advanced begin"));
  } else {
    Serial.println(F("Error initialising BH1750"));
  }

  float luxRead = lightMeter.readLightLevel();
  Serial.print("lux ");
  Serial.println(luxRead);
  delay(2000);
  float t12 = dht.readTemperature(); // Read temperature as Fahrenheit then dht.readTemperature(true)
  config.temp = t12;
  delay(2000);
  float h12 = dht.readHumidity();
  config.humid = h12;
  uint16_t soil = readSoil();
  config.soil = soil;
  uint32_t salt = readSalt();
  config.salt = salt;
  String advice;
  if (salt < 201) {
    advice = "needed";
  }
  else if (salt < 251) {
    advice = "low";
  }
  else if (salt < 351) {
    advice = "optimal";
  }
  else if (salt > 350) {
    advice = "too high";
  }
  Serial.println (advice);
  config.saltadvice = advice;



  float bat = readBattery();
  config.bat = bat;
  config.batcharge = "";
  if (bat > 130) {
    config.batcharge = "charging";
  }

  if (bat > 100) {
    config.bat = 100;
  }
  
  config.bootno = bootCount;


  luxRead = lightMeter.readLightLevel();
  Serial.print("lux ");
  Serial.println(luxRead);
  config.lux = luxRead;
  config.rel = rel;

  timeClient.setTimeOffset(gmtOffset_sec);
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  dayStamp = dayStamp.substring(5);
  String dateMonth = dayStamp.substring(0, 2);
  String dateDay = dayStamp.substring(3, 5);
  dayStamp = dateDay + "-" + dateMonth;
  config.date = dayStamp;
  // Extract time
  timeStamp1 = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
  config.time = timeStamp1.substring(0, 5);
  // variables needed for DST test
  int thisHour = timeClient.getHours();
  int thisDay = dateDay.toInt();
  int thisMonth = dateMonth.toInt();
  int thisWeekday = timeClient.getDay();
  bool dst = false;

  // Test for DST active
  if (thisMonth == 10 && thisDay < 25 && thisWeekday < 7 )  {
    dst = true;
  }

  if (thisMonth == 10 && thisDay > 24 && thisWeekday == 7 && thisHour < 2)  {
    dst = true;
  }

  if (thisMonth < 10 && thisMonth > 3) {
    dst = true;
  }

  if (thisMonth == 3) {
    dst = true;
    if (thisDay < 25) {
      dst = false;
    }
    else
      // thisDay > 25
    {
      if (thisWeekday == 7 && thisHour < 2)      {
        dst = false;
      }
      else {
        if (thisWeekday == 7) {
          dst = true;
        }
        else {
          if (thisWeekday < 7) {
            int checkSum = thisDay - thisWeekday + 7;
            if (checkSum > 31) {
              dst = true;
            }
            else {
              dst = false;
            }
          }
        }
      }
    }
  }

  if (dst) {
    Serial.println("IN SOMMERTIME");
    timeClient.setTimeOffset(gmtOffset_sec + 3600);
    while (!timeClient.update()) {
      timeClient.forceUpdate();
    }
    // The formattedDate comes with the following format:
    // 2018-05-28T16:00:13Z
    // We need to extract date and time
    formattedDate = timeClient.getFormattedDate();
    // Extract date
    int splitT = formattedDate.indexOf("T");
    dayStamp = formattedDate.substring(0, splitT);
    dayStamp = dayStamp.substring(5);
    String dateMonth = dayStamp.substring(0, 2);
    String dateDay = dayStamp.substring(3, 5);
    dayStamp = dateDay + "-" + dateMonth;
    timeStamp1 = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
  } else {
    Serial.println("IN VINTERTIME");
  }


  // Create JSON file
  Serial.println(F("Creating JSON document..."));
  if (logging) {
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

void loop() {
}
