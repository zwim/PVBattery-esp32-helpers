
#include <BluetoothSerial.h>

#include "config.h"

#define MY_NAME "ANT-Connector" __DATE__ " " __TIME__

// Check if Bluetooth is available
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
  #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Check Serial Port Profile
#if !defined(CONFIG_BT_SPP_ENABLED)
  #error Serial Port Profile for Bluetooth is not available or not enabled. It is only available for the ESP32 chip.
#endif

namespace bluetooth 
{
    bool enabled = true;
    bool connected = false;

    //#define USE_NAME // Comment this to use MAC address instead of a slaveName
    String myName = MY_NAME;
    const char *pin = "1234";

    #ifdef USE_NAME
    String slaveName = "BMS-ANT24S-666"; // Change this to reflect the real name of your slave BT device
    #else
    uint8_t address[6]  = {0xAA, 0xBB, 0xCC, 0xD0, 0x06, 0x66}; // Change this to reflect real MAC address of your slave BT device
    #endif

    BluetoothSerial SerialBT;

    bool init()
    {
        if (!enabled) 
            return false;

        if (strlen(config::bt_mac_address) < 17)
            return false;


        for (int i = 0; i < 6; ++i)
        {
            char tmp[4];
            strncpy(tmp, config::bt_mac_address + i*3 , 2);
            address[i] = strtol(tmp, NULL, 16);
        }
    
        bool isPinOk = false;
        isPinOk = SerialBT.setPin(pin, 4);  
        Serial.printf("PIN was set: %s\n", isPinOk ? "OK" : "ERROR");

        SerialBT.begin(myName, true);
        //SerialBT.deleteAllBondedDevices(); // Uncomment this to delete paired devices; Must be called after begin
        Serial.printf("The device \"%s\" started in master mode, make sure slave BT device is on!\n", myName.c_str());

        Serial.println("Start connecting ...");

        #ifndef USE_NAME
            isPinOk = SerialBT.setPin(pin, 4);  
            Serial.printf("PIN was set: %s\n", isPinOk ? "OK" : "ERROR");

            Serial.println("Using PIN"); 
        #endif


        // connect(address) is fast (up to 10 secs max), connect(slaveName) is slow (up to 30 secs max) as it needs
        // to resolve slaveName to address first, but it allows to connect to different devices with the same name.
        // Set CoreDebugLevel to Info to view devices Bluetooth address and device names
        #ifdef USE_NAME
            connected = SerialBT.connect(slaveName);
            Serial.printf("Connecting to slave BT device named \"%s\"\n", slaveName.c_str());
        #else
            connected = SerialBT.connect(address);
            Serial.print("Connecting to slave BT device with MAC "); Serial.println(config::bt_mac_address);
        #endif

        if (connected) {
            Serial.println("Bluetooth connected successfully!");
        } else {
            if (!SerialBT.connected(10000)) 
                Serial.println("Failed to initial connect. Make sure remote device is available and in range, then restart app.");
            else
                connected = true;
        }
        return connected;
    }

    bool writeBlockBT(const uint8_t buffer[], int size)
    {
        Serial.printf("writeBlockBT: %d\n\r", size);
        if (!enabled)
            return false;
    
        if (!connected)
        {
            init();
            if (!connected)
                return false;
        }
        Serial.printf("writeBlockBT: %d\n\r", size);
        bluetooth::SerialBT.write(buffer, size);
        bluetooth::SerialBT.flush();
        return true;
    }

    int readBlockBT(uint8_t buffer[], int max_size)
    {
        Serial.printf("readBlockBT: %d\n\r", max_size);
        if (!enabled)
            return -1;
        if (!connected)
        {
            init();
            if (!connected)
                return -1;
        }
        Serial.printf("readBlockBT: %d\n\r", max_size);
        int size = 0;
        while (bluetooth::SerialBT.available()) 
        {
            buffer[size++] = bluetooth::SerialBT.read();
            if (size > max_size)
                return -1;
            delay(1);
        }
        Serial.printf("readBlockBT: %d\n\r", size);
        return size;
    }
}