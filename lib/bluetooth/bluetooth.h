
#include <BluetoothSerial.h>

// Check if Bluetooth is available
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
//  #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Check Serial Port Profile
#if !defined(CONFIG_BT_SPP_ENABLED)
//  #error Serial Port Profile for Bluetooth is not available or not enabled. It is only available for the ESP32 chip.
#endif

namespace bluetooth 
{
    extern bool enabled;
    extern bool connected;
    
    extern String myName;
    extern BluetoothSerial SerialBT;
    
    bool init();
    bool writeBlockBT(const uint8_t buf[], int size);
    int readBlockBT(uint8_t buffer[], int max_size);
}