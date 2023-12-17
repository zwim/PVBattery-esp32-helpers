#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define VERSION "1.0"

namespace config
{
    extern bool shouldSaveConfig;

    extern char hostname[40];
    extern char bt_enabled[20];
    extern char bt_mac_address[40];
    extern char rs485_enabled[20];

    bool readConfigFile();
    void saveConfigFile(bool force = false);
}

#endif