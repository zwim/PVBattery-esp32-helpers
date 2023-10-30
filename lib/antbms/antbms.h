


#ifndef ANTBMS_H
#define ANTBMS_H

#include <stdint.h>
#include <Arduino.h>

#define empty_statement ;
#define BATTERY_CELL_COUNT 32

typedef int tristate;

struct BMSStruct {
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
	uint16_t
		temperatures[6];
	uint32_t
		currentPower,
		balancingFlags,
		totalTime;
};

class AntBMS {
	private:
		uint32_t lastData = 0;
	public:
		BMSStruct values;
		int requestData(uint8_t buffer[], int size);
		bool processPacket(uint8_t buffer[], int size);
		bool isChecksumValid(uint8_t buffer[], int size);

		tristate toggleAutoBalance();
		tristate readAutoBalance();
		tristate setAutoBalance(bool on);

		void reboot();

		void printSerial();
};

extern AntBMS bms;

#endif
