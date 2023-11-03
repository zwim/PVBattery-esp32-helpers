

// see https://github.com/imval/AntBMS for ANT-BMS
// see "klotztech vbms" -> Wiki for details

#define emptyStatement ;

#include "Arduino.h"
#include <WiFiManager.h> 

#include "antbms.h"
#include "bluetooth.h"
#include "config.h"
#include "soyosource.h"
#include "timeclient.h"
#include "webserver.h"

void setup() 
{
    Serial.begin(115200);
    for (int i = 20; i>0; i--)
        Serial.print("#");
    Serial.println(" " __DATE__ __TIME__);

    Serial.println("BOOTED!\n\n");

    Serial.println("Load config ...");
    config::readConfigFile();
    config::saveConfigFile(true); 
    Serial.println("Load config done.");

    Serial.println("Soyosource init ...");
    soyosource::init();
    Serial.println("Soyosource init done.");

    Serial.println("Bluetooth init ...");
    bluetooth::init();
    Serial.println("Bluetooth init done.");

    Serial.println("Ant-BMS init ...");
    bms.requestAndProcess();
    Serial.println("Ant-BMS init done.");

    Serial.println("Webserver init ...");
    webServer::init();
    Serial.println("Webserver init done.");

    Serial.println("NTP init ...");
    timeClient::init("europe.pool.ntp.org");
    timeClient::printLocalTime();
    Serial.println("NTP init done.");

} // end setup()


void loop() 
{
    delay(100);
}
