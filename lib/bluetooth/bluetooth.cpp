
#include <BluetoothSerial.h>

#define MY_NAME "ESP32-BT-Master";

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
    //#define USE_NAME // Comment this to use MAC address instead of a slaveName
    String myName = MY_NAME;
    const char *pin = "1234";

    bool connected = false;

    #ifdef USE_NAME
    String slaveName = "BMS-ANT24S-666"; // Change this to reflect the real name of your slave BT device
    #else
    String MACadd = "AA:BB:CC:D0:06:66"; // This only for printing
    uint8_t address[6]  = {0xAA, 0xBB, 0xCC, 0xD0, 0x06, 0x66}; // Change this to reflect real MAC address of your slave BT device
    #endif

    BluetoothSerial SerialBT;

    bool init()
    {
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
            Serial.print("Connecting to slave BT device with MAC "); Serial.println(MACadd);
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
}