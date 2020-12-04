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

