
#define empty_statement ;

#define FORMAT_LITTLEFS_IF_FAILED true
// You only need to format the filesystem once
// #define FORMAT_FILESYSTEM       true
#define FORMAT_FILESYSTEM false

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h> 
#include <ESPAsyncWebServer.h> 
#include <AsyncTCP.h>
#include "FS.h"
#include <LittleFS.h>
#include "secrets.h"
#include "sunrise.h"
#include "antbms.h"

WiFiServer WifiServer(23);
WiFiClient WifiServerClient; // for OTA
AsyncWebServer Server(80);

#ifdef test_debug
const char ESP_Hostname[] = "Battery_Control_ESP32_TEST"; // Battery_Control_ESP32
#else
const char ESP_Hostname[] = "Battery_Control_ESP32"; // Battery_Control_ESP32
#endif

extern uint8_t buffer[];

namespace webServer
{
    int g_CurrentChannel;
    int ActualPower = 0; // for current Power out of current clamp
    float ActualVoltage = 0;
    float ActualCurrent = 0;
    int ActualSetPower = 0;
    int ActualSetPowerInv = 0;
    int ActualSetPowerCharger = 0;
    float ActualSetVoltage = 56.2;
    float ActualSetCurrent = 0;
    int PowerReserveCharger = 15;
    int PowerReserveInv = 15;
    int MaxPowerCharger = 2000;
    int MaxPowerInv = 100;
    int DynPowerInv = 800;
    bool g_EnableCharge = true;
    float solar_prognosis;
    int target_SOC;              // [%]
    float BatteryCapacity = 4.6; // [kWh]

    char date_today[9];    // = "yyyymmdd";
    char date_tomorrow[9]; // = "yyyymmdd";
    int date_dayofweek_today = 8;

    unsigned long g_Time500 = 0;
    unsigned long g_Time1000 = 0;
    unsigned long g_Time5000 = 0;
    unsigned long g_Time30Min = 0;

    char temp_char[20]; // for temporary storage of strings values

    // Time information
    TimeStruct myTime; // create a TimeStruct instance to hold the returned data
    float time_now;
    bool is_day; // true: is day, false: is night

    // BMS values as structure

    // byte receivedBytes_main[320];

    // flag for saving data inside WifiManager
    bool shouldSaveConfig = true;

    // callback notifying us of the need to save config
    void saveConfigCallback()
    {
        Serial.println("Should save config");
        shouldSaveConfig = true;
    }

    // save configuration parameter like mqtt server, max inverter power
    void saveconfigfile()
    {
        // save the custom parameters to FS
        if (shouldSaveConfig)
        {
            Serial.println("saving config");
#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
            DynamicJsonDocument json(1024);
#else
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.createObject();
#endif
            json["cur_ip"] = current_clamp_ip;
            json["cur_cmd"] = current_clamp_cmd;
            json["sensor_resp"] = sensor_resp;
            json["sensor_resp_power"] = sensor_resp_power;
//            json["wm_resetsetting"] = wm_resetsetting;
            json["PowerReserveCharger"] = PowerReserveCharger;
            json["PowerReserveInv"] = PowerReserveInv;
            json["MaxPowerCharger"] = MaxPowerCharger;
            json["MaxPowerInv"] = MaxPowerInv;
//            json["g_enableDynamicload"] = g_enableDynamicload;

            File configFile = LittleFS.open("/config.json", "w");
            if (!configFile)
            {
                Serial.println("failed to open config file for writing");
            }
            else
            {
    #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
                serializeJson(json, Serial);
                serializeJson(json, configFile);
    #else
                json.printTo(Serial);
                json.printTo(configFile);
    #endif
                configFile.close();        
            }
        }
    }

    void init()
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

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
                    DynamicJsonDocument json(1024);
                    auto deserializeError = deserializeJson(json, buf.get());
                    serializeJson(json, Serial);
                    if (!deserializeError)
                    {
#else
                    DynamicJsonBuffer jsonBuffer;
                    JsonObject &json = jsonBuffer.parseObject(buf.get());
                    json.printTo(Serial);
                    if (json.success())
                    {
#endif
                        Serial.println("\nparsed json");
                        strcpy(current_clamp_ip, json["cur_ip"]);
                        strcpy(current_clamp_cmd, json["cur_cmd"]);
                        strcpy(sensor_resp, json["sensor_resp"]);
                        strcpy(sensor_resp_power, json["sensor_resp_power"]);
                        PowerReserveCharger = json["PowerReserveCharger"];
                        PowerReserveInv = json["PowerReserveInv"];
                        MaxPowerCharger = json["MaxPowerCharger"];
                        MaxPowerInv = json["MaxPowerInv"];
//                        wm_resetsetting = json["wm_resetsetting"];
//                        g_enableDynamicload = json["g_enableDynamicload"];
                    }
                    else
                    {
                        Serial.println("failed to load json config");
                    }
                    configFile.close();
                }
            }
        }
        else
        {
            Serial.println("failed to mount FS");
        } // end read

        Serial.print("Scan for SSIDs ... ");
        int n = WiFi.scanNetworks();
        Serial.printf("%d network(s) found\n", n);
        for (int i = 0; i < n; i++)
        {
            Serial.println(WiFi.SSID(i));
        }
        Serial.println();

        // The extra parameters to be configured (can be either global or just in the setup)
        // After connecting, parameter.getValue() will get you the configured value
        // id/name placeholder/prompt default length
        WiFiManagerParameter custom_current_clamp_ip("cur_ip", "current_clamp_ip", current_clamp_ip, 40);
        WiFiManagerParameter custom_current_clamp_cmd("cur_cmd", "current_clamp_command", current_clamp_cmd, 40);
        WiFiManagerParameter custom_sensor_resp("sensor_resp", "sensor_resp", sensor_resp, 20);
        WiFiManagerParameter custom_sensor_resp_power("sensor_resp_power", "sensor_resp_power", sensor_resp_power, 20);

        // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
        WiFiManager wm;
        wm.setWiFiAutoReconnect(true);

        //sets timeout until configuration portal gets turned off useful to make it all retry or go to sleep
        //in seconds
//        wm.setConfigPortalTimeout(180);
  
        // set config save notify callback
        wm.setSaveConfigCallback(saveConfigCallback);

        // add all your parameters here
        wm.addParameter(&custom_current_clamp_ip);
        wm.addParameter(&custom_current_clamp_cmd);
        wm.addParameter(&custom_sensor_resp);
        wm.addParameter(&custom_sensor_resp_power);

        // reset settings - wipe stored credentials for testing
        // these are stored by the esp library
        // wm.resetSettings();
//        if (wm_resetsetting)
//        {
//            wm.resetSettings();
//            wm_resetsetting = false;
//        }

        // Automatically connect using saved credentials,
        // if connection fails, it starts an access point with the specified name ("AutoConnectAP"),
        // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
        // then goes into a blocking loop awaiting configuration and will return success result

        bool res;
        // res = wm.autoConnect(); // auto generated AP name from chipid
        // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
        res = wm.autoConnect("AutoConnectAP", "password"); // password protected ap with password as password

        if (!res)
        {
            Serial.println("Failed to connect");
            // ESP.restart();
        }
        else
        {
            // if you get here you have connected to the WiFi
            Serial.println("WiFi connected... :)");
        }

        Serial.println("The values in the file are: ");
        Serial.println("\tcurrent clamp : " + String(current_clamp_ip));
        Serial.println("\tcommand : " + String(current_clamp_cmd));
        Serial.println("\tResponse  : " + String(sensor_resp));
        Serial.println("\tResponse power : " + String(sensor_resp_power));

        saveconfigfile();

        ArduinoOTA.onStart([]()
        {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else // U_SPIFFS
                type = "filesystem";

            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
            Serial.println("Start updating " + type); })
                .onEnd([]()
                    { Serial.println("\nEnd"); })
                .onProgress([](unsigned int progress, unsigned int total)
                            { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
                .onError([](ota_error_t error)
                        {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed"); 
        });

        ArduinoOTA.begin();
        Serial.println("\n########### ArduinoOTA.begin()");

        WifiServer.begin();

        Serial.println("Soyosource inverter RS485 setup done");

        Serial.println("JK-BMS RS485 setup done");

        // initialize NTP connection
        setuptimeClient();
        Serial.println("NTP setup done");

        Serial.println("Init done: ATTENTION MY_MYMYMYMMY");


        Server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(200, "text/plain", "ROOT-Folder not ready ...");
        });

/*        Server.on("/balance.on", HTTP_GET, [](AsyncWebServerRequest *request){
            AsyncResponseStream *response = request->beginResponseStream("text/plain", 150);
            if (bms.setAutoBalance(true))
                response->printf("on");
            else
                response->printf("off");
            request->send(response);
        });

        Server.on("/balance.off", HTTP_GET, [](AsyncWebServerRequest *request){
            AsyncResponseStream *response = request->beginResponseStream("text/plain", 150);
            if (bms.setAutoBalance(false))
                response->printf("off");
            else
                response->printf("on");
            request->send(response);
        });
        */

        Server.on("/balance.toggle", HTTP_GET, [](AsyncWebServerRequest *request){
            AsyncResponseStream *response = request->beginResponseStream("text/plain", 150);
            if (bms.toggleAutoBalance())
                response->printf("auto balance toggled on");
            else
                response->printf("auto balance toggled off");
            request->send(response);
        });

        Server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
            bms.reboot();
        });

        Server.on("/bms.data", HTTP_GET, [](AsyncWebServerRequest *request){
            AsyncResponseStream *response = request->beginResponseStream("application/octet-stream", 150); // todo use BUFFER_SIZE
            for (int i=0; i<140; ++i)
                response->printf("%c", buffer[i]);
            request->send(response);
        });

        Server.on("/show", HTTP_GET, [](AsyncWebServerRequest *request){
            AsyncResponseStream *response = request->beginResponseStream("text/plain", 1024); // todo use BUFFER_SIZE
            response->printf("Power: %lu W\n", bms.values.currentPower);
            response->printf("Current: %3.2f A\n", bms.values.current);
            response->printf("SOC: %d %%\n", bms.values.percentage);
            response->printf("totalCapacity: %5.3f Ah\n", bms.values.totalCapacity);
            response->printf("remainingCapacity: %5.3f Ah\n", bms.values.remainingCapacity);
            response->printf("chargeFlag: %x\n", bms.values.chargeFlag);
            response->printf("dischargeFlag: %x\n", bms.values.dischargeFlag);

            for (int probe = 0; probe < 6; probe++) {
                response->printf("Probe %d: %d °C\n", probe, bms.values.temperatures[probe]);
            }

            for (int cell = 0; cell < bms.values.numberOfBatteries; cell++) {
                response->printf("Cell %d: %1.3f\n", cell, bms.values.voltages[cell]);
            }

            request->send(response);
        });

        Server.begin();
    } // end init

    void looper()
    {
        // CAN handling and OTA shifted to core 1
        if ((millis() - g_Time500) > 500)
        {
            g_Time500 = millis();
        }

        if ((millis() - g_Time1000) > 1000)
        {

            // update the value for the inverter every second
//xx            sendpower2soyo(ActualSetPowerInv, Soyo_RS485_PORT_EN);
            g_Time1000 = millis();
        }
    }

} // end webServer
