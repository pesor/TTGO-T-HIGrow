void mqttSetup(String identyfikator, String chipId, String uom = "x", String dc = "x" )
{
  bool print_info = false;

  const String topicStr_c = mqttprefix + plant_name + "-" + chipId + "/" + identyfikator +"/config";
  const char* topic_c = topicStr_c.c_str();

  StaticJsonDocument<1536> doc_c;
  JsonObject root = doc_c.to<JsonObject>();

  root["name"] = plant_name +" "+ identyfikator;

  if ( dc != "x" ) {
    root["device_class"] = dc;
  }

  root["unique_id"] = chipId +"-"+ identyfikator;
  root["state_topic"] = mqttprefix + plant_name + "-" + chipId  + "/status";
  root["value_template"] = "{{ value_json['" + identyfikator +"'] }}";
  if ( uom != "x" ) {
    root["unit_of_measurement"] = uom;
  }

// // Send to mqtt
  char buffer_c[1536];
  serializeJson(doc_c, buffer_c);


  if (logging) {
    writeFile(SPIFFS, "/error.log", "Sending message to topic: \n");
  }

// nice print of configuration mqtt message
if (print_info) {
    Serial.println("*****************************************" );
    Serial.println(topic_c);
    Serial.print("Sending message to topic: \n");
    serializeJsonPretty(doc_c, Serial);
    Serial.println();
}

   bool retained = true;

  if (mqttClient.publish(topic_c, buffer_c, retained)) {
    if (print_info) {
      Serial.println("Message published successfully");
    }
  } else {
    if (print_info) {
        Serial.println("Error in Message, not published");
    }
    goToDeepSleepFiveMinutes();
  }

  if (print_info) {
    Serial.println("*****************************************\n" );
  }

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


 // Connect to mqtt broker
  Serial.print("Attempting to connect to the MQTT broker: ");
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Attempting to connect to the MQTT broker! \n");
  }

  Serial.println(broker);
  mqttClient.setServer(broker, port);

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

//https://www.home-assistant.io/integrations/sensor/#device-class
//Home Assitant MQTT Autodiscovery mesasaages
  mqttSetup("daysOnBattery",  chipId, "x",  "duration");
  mqttSetup("battvoltage",    chipId, "V",  "voltage");
  mqttSetup("bat",            chipId, "%",  "battery");
  mqttSetup("lux",            chipId, "lx", "illuminance");
  mqttSetup("humid",          chipId, "%",  "humidity");
  mqttSetup("soil",           chipId, "%",  "humidity");
  mqttSetup("salt",           chipId, "x");
  mqttSetup("temp",           chipId, "°C", "temperature");
  mqttSetup("RSSI",           chipId, "dBm", "signal_strength");
 
  const String topicStr = mqttprefix + plant_name + "-" + chipId + "/status";
  const char* topic = topicStr.c_str();

  StaticJsonDocument<1536> doc;
  // Set the values in the document
  // Device changes according to device placement
  JsonObject plant = doc.to<JsonObject>();
 
  // JsonObject plant = root.createNestedObject("sensor");
  plant["time"] = config.time;
  plant["battvolt"] = config.batvolt; //nie
  plant["battvoltage"] = config.batvoltage;  
  plant["bat"] = config.bat;
  plant["sleep5Count"] = sleep5no;
  plant["bootCount"] = config.bootno;  
  plant["device_name"] = device_name;
  plant["chipId"] = chipId;
  plant["sensorname"] = plant_name;
  plant["date"] = config.date;
  plant["batchargeDate"] = config.batchargeDate;
  plant["daysOnBattery"] = config.daysOnBattery;
  // plant["batcharge"] = config.batcharge; //nie
  plant["lux"] = config.lux; //nie
  plant["temp"] = config.temp;
  plant["humid"] = config.humid;
  // plant["pressure"] = config.pressure;
  plant["soil"] = config.soil;
  // plant["soilTemp"] = config.soilTemp; //nie
  plant["salt"] = config.salt;
  // plant["saltadvice"] = config.saltadvice;//nie
  // plant["plantValveNo"] = plantValveNo; //nie
  // plant["wifissid"] = WiFi.SSID(); //nie
  plant["rel"] = config.rel;
  plant["millis"] = millis() - setupstart;
  plant["RSSI"] = WiFi.RSSI(); //wifiRSSI;

  // Send to mqtt
  char buffer[1536];
  serializeJson(doc, buffer);


  Serial.print("Sending message to topic: ");
  Serial.println(topic);
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Sending message to topic: \n");
  }

  // Serial.println(buffer);
  serializeJsonPretty(doc, Serial);
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

