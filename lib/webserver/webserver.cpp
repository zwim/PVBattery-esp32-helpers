
#define empty_statement ;

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h> 
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h> 
#include <LittleFS.h>
#include <WiFi.h>
#include <ESP32Ping.h>

#include "antbms.h"
#include "config.h"
#include "secrets.h"
#include "soyosource.h"
#include "timeclient.h"
#include "util.h"
#include "webserver.h"

namespace webServer
{
    WiFiServer WifiServer(23);
    WiFiClient WifiServerClient; // for OTA
    AsyncWebServer Server(80);
    IPAddress local_ip;
    IPAddress gateway_ip;
    char gateway[20] = "";

    void initWebserverFunctions();
    bool preprocessResponse(char buf[], int buflen);

    // callback notifying us of the need to save config
    void saveConfigCallback()
    {
        Serial.println("Should save config");
        config::shouldSaveConfig = true;
    }

    void init()
    {
        Serial.print("Scan for SSIDs ... ");
        int n = WiFi.scanNetworks();
        Serial.printf("%d network(s) found\n", n);
        for (int i = 0; i < n; i++)
            Serial.println(WiFi.SSID(i));
        Serial.println();

        // The extra parameters to be configured (can be either global or just in the setup)
        // After connecting, parameter.getValue() will get you the configured value
        // id/name placeholder/prompt default length
        WiFiManagerParameter custom_current_hostname("cur_hostname", "Hostname", config::hostname, sizeof(config::hostname));
        WiFiManagerParameter custom_current_bt_enabled("cur_bt_enabled", "Bluetooth enabled (ANT)", config::bt_enabled, sizeof(config::bt_enabled));
        WiFiManagerParameter custom_current_rs485_enabled("cur_rs485_enabled", "RS485 enabled (SOYOSOURCE)", config::rs485_enabled, sizeof(config::rs485_enabled));

        // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
        WiFiManager wm;
        
        // wm.setConnectTimeout(15); 
        wm.setWiFiAutoReconnect(true);

        //sets timeout until configuration portal gets turned off useful to make it all retry or go to sleep
        //in seconds
//        wm.setConfigPortalTimeout(180);
  
        // set config save notify callback
        wm.setSaveConfigCallback(saveConfigCallback);

        // add all your parameters here
        wm.addParameter(&custom_current_hostname);
        wm.addParameter(&custom_current_bt_enabled);
        wm.addParameter(&custom_current_rs485_enabled);


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
        Serial.println("\tcurrent_hostname: " + String(config::hostname));
        Serial.println("\tcurrent_bt_enabled: " + String(config::bt_enabled));
        Serial.println("\tcurrent_rs485_enabled: " + String(config::rs485_enabled));

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

        Serial.println("\n########### ArduinoOTA.begin()");
        ArduinoOTA.begin();

        Serial.println("\n########### WifiServer.begin()");
        WifiServer.begin();

        initWebserverFunctions();
        Server.begin();

        local_ip = WiFi.localIP();
        Serial.print("IP: "); Serial.println(local_ip);
        gateway_ip = WiFi.gatewayIP();
        Serial.print("GATEWAY: "); Serial.println(gateway_ip);
        snprintf(gateway, sizeof(gateway), "%d.%d.%d.%d", gateway_ip[0], gateway_ip[1], gateway_ip[2], gateway_ip[3]);
    } // end init

    void initWebserverFunctions()
    {
        Server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
            AsyncResponseStream *response = request->beginResponseStream("text/plain", 1024); // todo use BUFFER_SIZE

            response->printf("Hostname: %s\n", config::hostname);
            response->printf("Target Bluetooth Adress: %s\n", config::bt_mac_address);
            response->printf("Bluetooth enabled: %s\n", config::bt_enabled[0] == '0' ? "off" : "on");
            response->printf("RS485 enabled: %s\n", config::rs485_enabled[0] == '0' ? "off" : "on");
            response->printf("%s\n", timeClient::showLocalTime());

            response->printf("\nBuild: %s %s\n\n", __DATE__, __TIME__);

            response->printf("/show\n");
            response->printf("/bms.data\n");
            response->printf("/balance.toggle\n");
            response->printf("/restart\n");
            response->printf("set?power=int, set?bms_discharge=0|1, set?btmac=xxxx, set?bluetooth=0|1, set?rs485=0|1");

            response->printf("/power/watt  watt may be a positive number\n");
            response->printf("/reboot\n");

            request->send(response);
        });

/*
        Server.on("/balance.on", HTTP_GET, [](AsyncWebServerRequest *request){
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
            tristate retval = bms.toggleAutoBalance();
            if (retval == 1)
                response->printf("auto balance toggled on");
            else if (retval == -1)
                response->printf("auto balance toggled off");
            else
                response->printf("auto balance toggle failed");
            request->send(response);
        });

        Server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request){
            bms.reboot();
        });

        Server.on("/bms.data", HTTP_GET, [](AsyncWebServerRequest *request){
            AsyncResponseStream *response = request->beginResponseStream("application/octet-stream", 150); // todo use BUFFER_SIZE
            if (bms.requestAndProcess())
            {
                for (int i=0; i<140; ++i)
                    response->printf("%c", bms.buffer[i]);
            }
            else
            {
                response->printf("Error %c\n", 0x66);
            }
            request->send(response);
        });

        Server.on("/show", HTTP_GET, [](AsyncWebServerRequest *request){
            AsyncResponseStream *response = request->beginResponseStream("text/plain", 1024); // todo use BUFFER_SIZE
            bms.requestAndProcess();
            response->printf(VERSION " " __DATE__ " " __TIME__ "\n");
            response->printf("Power: %ld W\n", bms.values.currentPower);
            response->printf("Current: %3.2f A\n", bms.values.current);
            response->printf("SOC: %d %%\n", bms.values.percentage);
            response->printf("totalCapacity: %5.3f Ah\n", bms.values.totalCapacity);
            response->printf("remainingCapacity: %5.3f Ah\n", bms.values.remainingCapacity);
            response->printf("chargeFlag: 0x%x\n", bms.values.chargeFlag);
            if (bms.values.dischargeFlag == 0x01)
                response->printf("dischargeFlag: open\n\n");
            else if (bms.values.dischargeFlag == 0x0f)
                response->printf("dischargeFlag: manually close\n\n");
            else
                response->printf("dischargeFlag: 0x%x\n\n", bms.values.dischargeFlag);

            response->printf("highest Cell [%2d]: %5.3f V\n", bms.values.highestNumber, bms.values.highestVoltage);
            response->printf("lowest Cell  [%2d]: %5.3f V\n", bms.values.lowestNumber, bms.values.lowestVoltage);
            response->printf("Cell diff          %5.3f V\n", bms.values.highestVoltage - bms.values.lowestVoltage);
            response->printf("Balancers: (0x%X)\n", (unsigned int) bms.values.balancingFlags);

            for (int cell = 0; cell < bms.values.numberOfBatteries; cell++) {
                response->printf("Cell %2d%c %1.3f V \n", cell+1, bms.values.balancingFlags & (1<<cell) ? 'x' : ':', bms.values.voltages[cell]);
            }

            for (int probe = 0; probe < 6; probe++) {
                if (bms.values.temperatures[probe] < 200 && bms.values.temperatures[probe] > -20)
                    response->printf("Probe %d: %d C\n", probe, bms.values.temperatures[probe]);
                else
                    response->printf("Probe %d: not connected\n", probe);
            }

            request->send(response);
        });

        Server.onNotFound([](AsyncWebServerRequest *request){
            request->send(404, "text/plain", "Path not found. Try /");
        });

        /*
        // Send a GET request to <IP>/sensor/<number>
        Server.on("^\\/power\\/([0-9]+)$", HTTP_GET, [] (AsyncWebServerRequest *request) 
        {
            String requestedPower = request->pathArg(0);
            request->send(200, "text/plain", "Requested Power: " + requestedPower + " W");
            soyosource::requestPower(requestedPower.toInt());
        });
        */ 

        Server.on("/powerinc1", HTTP_GET, [](AsyncWebServerRequest *request){
            soyosource::requestPowerInc(+1);  
            Serial.println("power+1") ;
        });
        Server.on("/powerinc10", HTTP_GET, [](AsyncWebServerRequest *request){
            soyosource::requestPowerInc(+10);   
        });
        Server.on("/powerdec1", HTTP_GET, [](AsyncWebServerRequest *request){
            soyosource::requestPowerInc(-1);   
        });
        Server.on("/powerdec10", HTTP_GET, [](AsyncWebServerRequest *request){
            soyosource::requestPowerInc(-10);   
        });


        // Send a GET request to <IP>/set?power=<value>
        // set?power=int
        // set?bms_discharge=0|1
        // set?btmac=xxxx
        // set?bluetooth=0|1
        // set?rs485=0|1
        Server.on("/set", HTTP_GET, [] (AsyncWebServerRequest *request) {
            if (request->hasParam("power")) 
            {
                String requestedPower = request->getParam("power")->value();
                request->send(200, "text/plain", "Requested Power: " + requestedPower + " W");
                soyosource::requestPower(requestedPower.toInt());
            }
            else if (request->hasParam("bms_discharge")) 
            {
                String bms_discharge = request->getParam("bms_discharge")->value();
                request->send(200, "text/plain", "bms set to: " + bms_discharge);
                bms.setDischargeMos(bms_discharge.toInt());
            }
            else if (request->hasParam("btmac")) 
            {   // not tested yet
                String macadr = request->getParam("btmac")->value();
                request->send(200, "text/plain", "Bluetooth MAC: " + macadr);
                strcpy(config::bt_mac_address, macadr.c_str());
                config::saveConfigFile(true);
                delay(3000);
                ESP.restart();
            }
            else if (request->hasParam("bluetooth"))
            {
                String requestedPower = request->getParam("bluetooth")->value();
                if (requestedPower[0] == '1')
                {
                    request->send(200, "text/plain", "enable Bluetooth");
                    strcpy(config::bt_enabled, "1");
                    bluetooth::enabled = true;
                }
                else if (requestedPower[0] == '0')
                {
                    request->send(200, "text/plain", "disable Bluetooth");
                    strcpy(config::bt_enabled, "0");
                    bluetooth::enabled = false;
                }
                else
                    return;

                config::saveConfigFile(true);
                delay(3000);
                ESP.restart();
            } 
            else if (request->hasParam("rs485"))
            {
                String requestedPower = request->getParam("rs485")->value();
                if (requestedPower[0] == '1')
                {
                    request->send(200, "text/plain", "enable rs485");
                    strcpy(config::rs485_enabled, "1");
                    soyosource::enabled = true;
                }
                else if (requestedPower[0] == '0')
                {
                    request->send(200, "text/plain", "disable rs485");
                    strcpy(config::rs485_enabled, "0");
                    soyosource::enabled = false;
                }
                else
                    return;

                config::saveConfigFile(true);
                delay(3500);
                ESP.restart();
            } 
            else 
            {
                request->send(200, "text/plain", "nothing set");
            }
        });


        //Send index.html as text
        Server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
            struct 
            {
                const char* name; // filename in 
                bool enabled; // shall file be processed
            } files[] = { 
                {"/index.htm", true}, 
                {"/top.htm", true},
                {"/soyo.htm", soyosource::enabled},
                {"/ant.htm", bluetooth::enabled},
//                {"/mqtt.htm", true},
                {"/bottom.htm", true},
                {"/script.js", true},
                {0, 0},
            };

            size_t max_size = 0;
            for (int i = 0; files[i].name != 0; ++i)
            {    
                if (!files[i].enabled) continue;

                File file = LittleFS.open(files[i].name, "r");
                if (file)
                {
                    size_t size = file.size();
                    if (size > max_size)
                        max_size = size;
                }                
            }

            // Allocate a buffer to store contents of the file.
            char *buf = new char[max_size];

            AsyncResponseStream *response = request->beginResponseStream("text/html");
            response->addHeader("Server","ESP Async Web Server");

            for (int i = 0; files[i].name != 0; ++i)
            {    
                if (!files[i].enabled) continue;

                File file = LittleFS.open(files[i].name, "r");
                if (file)
                {
                    size_t size = file.size();
                    file.readBytes(buf, size);

                    preprocessResponse(buf, max_size);

                    char format_string[16];
                    snprintf(format_string, sizeof(format_string),"%%.%ds", (int)size);
                    response->printf(format_string, buf);
                    file.close();
                }
            }
            response->print("</body></html>");
            //send the response last
            request->send(response);

            delete[] buf;
        });
        
    } // end initWebserverFunctions

    bool preprocessResponse(char buf[], int buflen)
    {
        char naChars[] = "n.a.";
        char powerChars[16];
        snprintf(powerChars, sizeof(powerChars), "%d", soyosource::getLastPower());

        bms.requestAndProcess();

        char num_of_cells[20];
        snprintf(num_of_cells, sizeof(num_of_cells), "%d", bms.values.numberOfBatteries);

        char cell_val[32][50];
        for (int i = 0; i<32; ++i)
        {
            if (i < bms.values.numberOfBatteries) 
                snprintf(cell_val[i], 50, "<td>[%d] %5.3f V</td>", i, bms.values.voltages[i]);
            else
                snprintf(cell_val[i], 50, " ");
        }

        char client_id[16];
        snprintf(client_id, sizeof(client_id), "%d.%d.%d.%d", local_ip[0], local_ip[1], local_ip[2], local_ip[3]);

        struct 
        {
            const char* pattern;
            char* replacement;
        } pr[] = {
            {"%CLIENTIPADRESSxx%", client_id},
            {"%VERSION  Dec 13 2023 00:00:00%", VERSION " " __DATE__ " " __TIME__},
            {"%STATICPOWER%", powerChars},
            {"%UPTIME%", naChars},
            {"%NUMBER_OF_CELLS%", num_of_cells},
            {"%<td>[xx] CELL0 V</td>%", cell_val[0]},
            {"%<td>[xx] CELL1 V</td>%", cell_val[1]},
            {"%<td>[xx] CELL2 V</td>%", cell_val[2]},
            {"%<td>[xx] CELL3 V</td>%", cell_val[3]},
            {"%<td>[xx] CELL4 V</td>%", cell_val[4]},
            {"%<td>[xx] CELL5 V</td>%", cell_val[5]},
            {"%<td>[xx] CELL6 V</td>%", cell_val[6]},
            {"%<td>[xx] CELL7 V</td>%", cell_val[7]},
            {"%<td>[xx] CELL8 V</td>%", cell_val[8]},
            {"%<td>[xx] CELL9 V</td>%", cell_val[9]},
            {"%<td>[xx] CELL10 V</td>%", cell_val[10]},
            {"%<td>[xx] CELL11 V</td>%", cell_val[11]},
            {"%<td>[xx] CELL12 V</td>%", cell_val[12]},
            {"%<td>[xx] CELL13 V</td>%", cell_val[13]},
            {"%<td>[xx] CELL14 V</td>%", cell_val[14]},
            {"%<td>[xx] CELL15 V</td>%", cell_val[15]},
            {"%<td>[xx] CELL16 V</td>%", cell_val[16]},
            {"%<td>[xx] CELL17 V</td>%", cell_val[17]},
            {"%<td>[xx] CELL18 V</td>%", cell_val[18]},
            {"%<td>[xx] CELL19 V</td>%", cell_val[19]},
            {"%<td>[xx] CELL20 V</td>%", cell_val[20]},
            {"%<td>[xx] CELL21 V</td>%", cell_val[21]},
            {"%<td>[xx] CELL22 V</td>%", cell_val[22]},
            {"%<td>[xx] CELL23 V</td>%", cell_val[23]},
            {"%<td>[xx] CELL24 V</td>%", cell_val[24]},
            {"%<td>[xx] CELL25 V</td>%", cell_val[25]},
            {"%<td>[xx] CELL26 V</td>%", cell_val[26]},
            {"%<td>[xx] CELL27 V</td>%", cell_val[27]},
            {"%<td>[xx] CELL28 V</td>%", cell_val[28]},
            {"%<td>[xx] CELL29 V</td>%", cell_val[29]},
            {"%<td>[xx] CELL30 V</td>%", cell_val[30]},
            {"%<td>[xx] CELL31 V</td>%", cell_val[31]},
            {0, 0},
        };

        for (int i = 0; pr[i].pattern; ++i)
        {
            char *start;
            int pattern_len = strlen(pr[i].pattern);
            int replacement_len = strlen(pr[i].replacement);
            while (true)
            {
                start = strstr(buf, pr[i].pattern);
                if (!start)
                    break;
                for ( int j=0; j < replacement_len; ++j)
                    start[j] = pr[i].replacement[j];
                for ( int j = replacement_len; j < pattern_len; ++j)
                    start[j] = ' ';
            }
        }
        return true;
    }

    bool checkGateway()
    {
        Serial.println(webServer::gateway);
        bool success = Ping.ping(webServer::gateway, 2);
        if (success)
            Serial.println("Network OK");
        else
            Serial.println("No network");

        return success;
    }

} // end webServer
