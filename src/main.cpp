

// see https://github.com/imval/AntBMS for ANT-BMS

#define emptyStatement ;

#include "Arduino.h"
//#include <strings.h>

#define BUFFER_SIZE 150
uint8_t buffer[BUFFER_SIZE];


#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <webserver.h>
#include <bluetooth.h>
#include <antbms.h>

void setup() 
{
    for (int i = 149; i>=0; --i)
        buffer[i] = 'x';

    Serial.begin(115200);
    for (int i = 20; i>0; i--)
        Serial.print("#");
    Serial.println(__DATE__);

    Serial.println("BOOTED!\n\n");

    webServer::init();

    bluetooth::init();

} // end setup()


void loop() 
{

    // reading ANT-BMS data and process them
    int bytesRead = bms.requestData(buffer, BUFFER_SIZE);
    if (bytesRead > 0 )
    {
        bms.processPacket(buffer, BUFFER_SIZE);
        bms.printSerial();
    }

    // vTaskDelay(10);
    delay(100);

}
