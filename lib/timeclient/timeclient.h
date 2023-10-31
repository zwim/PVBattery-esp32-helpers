#ifndef TIMECLIENT_H
#define TIMECLIENT_H

#include <stdint.h>

namespace timeClient 
{
    extern char* ntpServer;
    extern uint32_t  ts_offset_s;
    extern uint16_t   dst_offset_s;

    void init(const char* server, uint32_t _tz_offset_s = 3600, uint16_t _dst_offset_s = 3600);
    const char* showLocalTime();

    inline void printLocalTime()
    {
        Serial.println(showLocalTime());
    }
}

#endif