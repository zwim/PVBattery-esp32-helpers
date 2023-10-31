

#ifndef ANTBMS_H
#define ANTBMS_H

#include <stdint.h>
#include <Arduino.h>

#include "bluetooth.h"

#define empty_statement ;
#define BATTERY_CELL_COUNT 32

typedef int tristate;

struct BMSStruct 
{
	float
		averageVoltage,
		capacity,
		current = 0.42,
		highestVoltage,
		lowestVoltage,
		remainingCapacity,
		totalCapacity,
		totalCycleCapacity,
		totalVoltage,
		voltages[BATTERY_CELL_COUNT],
		voltageSum,
		voltageDiff;
	uint8_t
		percentage, 
		chargeFlag,
		dischargeFlag,
		numberOfBatteries,
		balancedStatusFlag,
		realySwitch,
		highestNumber,
		lowestNumber,
		numberOfMonomerStrings;
	int16_t
		temperatures[6];
	uint32_t
		currentPower;
	uint32_t
		balancingFlags,
		totalTime;
};

class AntBMS 
{
	private:
		unsigned long lastDataMillis = 0;
		int requestData();
		bool processPacket();
		bool isChecksumValid();

		unsigned long dataAge() {
			unsigned long age = millis() - lastDataMillis;
			Serial.println(age);
			return age > 0 ? age : ((unsigned long) (-1L)) - age;
		}

	public:
		uint8_t buffer[150];  // at leas 150 bytes
		BMSStruct values;
		bool requestAndProcess();

		tristate toggleAutoBalance();
		tristate readAutoBalance();
		tristate setAutoBalance(bool on);

		void reboot();

		void printSerial();
};

extern AntBMS bms;

#endif
