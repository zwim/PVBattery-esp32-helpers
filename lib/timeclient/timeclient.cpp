#include <Arduino.h>
#include <time.h>

namespace timeClient 
{
    const char* ntpServer;
    uint32_t  tz_offset_s;
    uint16_t  dst_offset_s;
    char time_str[100];
    struct tm timeinfo;

    void init(const char* server, uint32_t _tz_offset_s, uint16_t _dst_offset_s)
    {
        ntpServer = server;

        timeClient::tz_offset_s = _tz_offset_s;
        timeClient::dst_offset_s = _dst_offset_s;

        //init and get the time
        configTime(_tz_offset_s, _dst_offset_s, ntpServer);

        if(!getLocalTime(&timeinfo)){
            Serial.println("Failed to obtain time");
        }
    }

    const char* showLocalTime()
    {
        if(!getLocalTime(&timeinfo)){
            Serial.println("Failed to obtain time");
        }
        int tm_year = timeinfo.tm_year;
        if (tm_year > 100)
            tm_year = tm_year - 100 + 2000;
        sprintf(time_str, "%4d/%02d/%02d-%02d:%02d:%02d", 
                        tm_year, timeinfo.tm_mon, timeinfo.tm_mday,
                        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

        return time_str;
    }
}