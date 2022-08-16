// READ Sensors

// READ Salt
// I am not quite sure how to read and use this number. I know that when put in water wich a DH value of 26, it gives a high number, but what it is and how to use ??????
uint32_t readSalt()
{
  uint8_t samples = 120;
  uint32_t humi = 0;
  uint16_t array[120];

  for (int i = 0; i < samples; i++)
  {
    array[i] = analogRead(SALT_PIN);
  //  Serial.print("Read salt pin : ");

  //  Serial.println(array[i]);
    delay(2);
  }
  std::sort(array, array + samples);
  for (int i = 0; i < samples; i++)
  {
    if (i == 0 || i == samples - 1)
      continue;
    humi += array[i];
  }
  humi /= samples - 2;
  return humi;
}

// READ Soil
uint16_t readSoil()
{
  //Serial.println(soil_max);
  uint16_t soil = analogRead(SOIL_PIN);
  Serial.print("Soil before map: ");
  Serial.println(soil);
  return map(soil, soil_min, soil_max, 100, 0);
}

float readSoilTemp()
{
  float temp;
  // READ Soil Temperature
  if (USE_18B20_TEMP_SENSOR)
  {
    //Single data stream upload
    temp = temp18B20.temp();
  }
  else
  {
    temp = 0.00;
  }
  return temp;
}

// READ Battery
float readBattery(uint16_t voltx)
{
  uint16_t volt;
  volt = voltx; //read at "setup" start


  Serial.print("\nVolt reading raw ");
  Serial.println(volt);
  config.batvolt = volt;

  // float battery_voltage = ((float)volt / 4095.0) * 2.0 * 3.3 * (vref) / 1000;
  // float battery_voltage =  0.001338217338 * volt +  0.948424908425 ;//volts
  float battery_voltage =  ( bat_volt_high - bat_volt_low ) / ( bat_reading_high - bat_reading_low ) * volt 
        +  bat_volt_high - bat_reading_high * ( bat_volt_high - bat_volt_low ) / ( bat_reading_high - bat_reading_low ) ;//true volts
  config.batvoltage = battery_voltage;

  Serial.print("Battery Voltage: ");
  Serial.println((float) battery_voltage);
  // Serial.println( 100 *  ( battery_voltage - 3.07) / ( 3.66 - 3.07 )) ;
  // battery_voltage = battery_voltage * 10000;
  // return map(battery_voltage, 36600, 30700, 10000, 0)/100;
  return 100 *  ( battery_voltage - bat_volt_low ) / ( bat_volt_high - bat_volt_low );//battery volt percents
}