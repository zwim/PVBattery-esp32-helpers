
#include <ArduinoJson.h>
#include "FS.h"
#include <LittleFS.h>

#include "bluetooth.h"
#include "config.h"
#include "util.h"
#include "soyosource.h"

#define FORMAT_LITTLEFS_IF_FAILED true
// You only need to format the filesystem once
// #define FORMAT_FILESYSTEM       true
#define FORMAT_FILESYSTEM false

namespace config
{
    // flag for saving data inside WifiManager
    bool shouldSaveConfig = false;

    char hostname[40] = "ANT_CONNECTOR";
    char bt_enabled[20] = "1";
    char bt_mac_address[40] = "AA:BB:CC:DD:EE:00";
    char rs485_enabled[20] = "1";

    // save configuration, max inverter power ...
    void saveConfigFile(bool force)
    {
        // save the custom parameters to FS
        if (shouldSaveConfig || force)
        {
            Serial.println("saving config");
            DynamicJsonDocument json(1024);
            json["cur_hostname"] = hostname;
            json["cur_bt_enabled"] = bt_enabled;
            json["cur_bt_mac_address"] = bt_mac_address;
            json["cur_rs485_enabled"] = rs485_enabled;

            File configFile = LittleFS.open("/config.json", "w");
            if (!configFile)
            {
                Serial.println("failed to open config file for writing");
            }
            else
            {
                serializeJson(json, Serial);
                serializeJson(json, configFile);
                configFile.close();        
            }
        }
    }

    bool readConfigFile()
    {
        // read configuration from FS json
        Serial.println("mounting FS...");

        if (LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED))
        {
            Serial.println("mounted file system");
            if (LittleFS.exists("/config.json"))
            {
                // file exists, reading and loading
                Serial.println("reading config file");
                File configFile = LittleFS.open("/config.json", "r");
                if (configFile)
                {
                    Serial.println("opened config file");
                    size_t size = configFile.size();
                    // Allocate a buffer to store contents of the file.
                    std::unique_ptr<char[]> buf(new char[size]);

                    configFile.readBytes(buf.get(), size);

                    DynamicJsonDocument json(1024);
                    auto deserializeError = deserializeJson(json, buf.get());
                    serializeJson(json, Serial);
                    if (!deserializeError)
                    {
                        Serial.println("\nparsed json");
                        strcpy(hostname, json["cur_hostname"]);
                        strcpy(bt_enabled, json["cur_bt_enabled"]);
                        if (json["curr_bt_mac_address"])
                            strcpy(bt_mac_address, json["cur_bt_mac_address"]);
                        else
                            strcpy(bt_mac_address, "AA:BB:CC:D0:06:66");

                        strcpy(rs485_enabled, json["cur_rs485_enabled"]); 

                        util::stringToLower(bt_enabled);
                        util::stringToLower(rs485_enabled);

                        if (util::isStringTrue(bt_enabled))
                        {
                            strcpy(bt_enabled, "1");
                            bluetooth::enabled = true;
                        }
                        else
                        {
                            strcpy(bt_enabled, "0");
                            bluetooth::enabled = false;
                        }

                        if (util::isStringTrue(rs485_enabled))
                        {
                            strcpy(rs485_enabled, "1");
                            soyosource::enabled = true;
                        }
                        else
                        {
                            strcpy(rs485_enabled, "0");
                            soyosource::enabled = false;
                        }
                    }
                    else
                    {
                        Serial.println("failed to load json config");
                    }
                    configFile.close();
                }
                File file_to_test;
                file_to_test = LittleFS.open("/index.html", "r");
                if (file_to_test)
                {
                    Serial.println("Found index.html");
                    file_to_test.close();
                }
                file_to_test = LittleFS.open("/index_ohne_bt.html", "r");
                if (file_to_test)
                {
                    Serial.println("Found index_ohne_bt.html");
                    file_to_test.close();
                }
            }
        }
        else
        {
            Serial.println("failed to mount FS");
            return false;
        } // end read
        return true;
    } 
}