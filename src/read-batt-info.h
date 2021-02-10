#include <Arduino.h>
// Read battery charging info
void read_batt_info()
{
    readFile(SPIFFS, "/batinfo.conf");
    Serial.println("Here comes the calibration info:");
    Serial.println(readString);
    if (logging)
    {
        String logInfo = "Batt charge info: " + String(readString) + " \n";
        const char *logInfo_write = logInfo.c_str();
        writeFile(SPIFFS, "/error.log", logInfo_write);
    }

    String xval = getValue(readString, ':', 0);
    String yval = getValue(readString, ':', 1);
    String zval = getValue(readString, ':', 2);

    battchargeDate = xval;
    battchargeDateCnt = yval.toInt();
    battchargeDateCntLast = zval;

    Serial.print("last cnt date ");
    Serial.println(battchargeDateCntLast);

    readString = "";
}