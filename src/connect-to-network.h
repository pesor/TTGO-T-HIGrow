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
  for (int i = 0; i < ssidArrNo; i++) {
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