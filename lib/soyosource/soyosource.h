
#ifndef SOYOSOURCE_C
#define SOYOSOURCE_C

/* 

    connection of the rs485 should be
    GPIO16 	U2RXD                  Max485 CSA
    GPIO17 	U2TxD                  Max485 CSA
    GPIO18 	RS485_EN Chip enable   Max485 CSA

#define Soyo_RS485_PORT_RX 16 //     -- RO
#define Soyo_RS485_PORT_TX 17 //     -- DI
#define Soyo_RS485_PORT_EN 18 //     -- DE/RE

*/

#include <stdint.h>

#define SOYO_RS485_PORT_RX 16 //     -- RO
#define SOYO_RS485_PORT_TX 17 //     -- DI
#define SOYO_RS485_PORT_EN 18 //     -- DE/RE

namespace soyosource
{
    extern bool enabled;
    extern bool connected;

    extern int16_t lastPower;

    // returns the last power set
    int getLastPower();

    // initialize RS485 bus for Soyosource communication
    void init(uint8_t rx_pin = SOYO_RS485_PORT_RX, uint8_t tx_pin = SOYO_RS485_PORT_TX, uint8_t en_pin = SOYO_RS485_PORT_EN);

    // Function prepares serial string and sends powerdemand by serial interface to Soyo
    // needs as input also the enable pin of the RS485 interface
    void requestPower(int16_t demandpowersend = 0, uint8_t en_pin = SOYO_RS485_PORT_EN);
    void requestPowerInc(int16_t inc,  uint8_t en_pin = SOYO_RS485_PORT_EN);
}

#endif
