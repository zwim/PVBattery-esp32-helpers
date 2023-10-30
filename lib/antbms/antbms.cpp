
#include "antbms.h"
#include <bluetooth.h>
#include "Arduino.h"

const uint8_t REQUEST_DATA[] = {0xDB, 0xDB, 0x00, 0x00, 0x00, 0x00};

int16_t buffer_get_int16(const uint8_t buffer[], uint8_t index) {
	return (((uint16_t) buffer[index]) << 8) | ((uint16_t) buffer[index + 1]);
}

int32_t buffer_get_int32(const uint8_t buffer[], uint8_t index) {
	return ((uint32_t) buffer[index]) << 24 | ((uint32_t) buffer[index + 1]) << 16 |
		((uint32_t) buffer[index + 2]) << 8 | ((uint32_t) buffer[index + 3]);
}

AntBMS bms;

bool AntBMS::processPacket(uint8_t buffer[], int size) {
	if (!isChecksumValid(buffer, size))
		return false;

	// Read stuff
	values.totalVoltage = (double) buffer_get_int16(buffer, 4) * 0.1;
	values.current = (double) buffer_get_int32(buffer, 70) * 0.1;
	values.percentage = (uint8_t) buffer[74];
	values.totalCapacity = (double) buffer_get_int32(buffer, 75) * 1e-6;
	values.remainingCapacity = (double) buffer_get_int32(buffer, 79) * 1e-6;
	values.totalCycleCapacity = (double) buffer_get_int32(buffer, 83) * 1e-3;
	values.totalTime = buffer_get_int32(buffer, 87);
	values.chargeFlag = (uint8_t) buffer[103];
	values.dischargeFlag = (uint8_t) buffer[104];
	values.balancedStatusFlag = (uint8_t) buffer[105];
	values.realySwitch = buffer[110];
	values.currentPower = buffer_get_int32(buffer, 111);
	values.highestNumber = (uint8_t) buffer[115];
	values.highestVoltage = (double) buffer_get_int16(buffer, 116) * 1e-3;
	values.lowestNumber = (uint8_t) buffer[118];
	values.lowestVoltage = (double) buffer_get_int16(buffer, 119) * 1e-3;
	values.voltageDiff = values.highestVoltage - values.lowestVoltage;
	values.averageVoltage = (double) buffer_get_int16(buffer, 121) * 1e-3;
	values.balancingFlags = buffer_get_int32(buffer, 132);
 	
	values.numberOfBatteries = (uint8_t) buffer[123];

	// Todo check if numberOfBatteries <= 32

	// Read cell voltages
	int cell = 0, i = 0;
	values.voltageSum = 0;
	for (i = 6; i < (6 + values.numberOfBatteries * 2); i += 2) {
		values.voltages[cell] = (double) buffer_get_int16(buffer, i) * 1e-3;
		values.voltageSum += values.voltages[cell];
		cell++;
	}

	// Temps
	int probe = 0;
	for (i = 91; i <= 102; i += 2) {
		values.temperatures[probe] = buffer_get_int16(buffer, i);
		probe++;
	}
	return true;
}

void AntBMS::printSerial()
{
	Serial.print("Current:"); Serial.println(values.current);
	Serial.print("Percentage:"); Serial.println(values.percentage);
	Serial.print("totalCapacity:"); Serial.println(values.totalCapacity);
	Serial.print("remainingCapacity:"); Serial.println(values.remainingCapacity);
	Serial.print("totalCapacity:"); Serial.println(values.totalCapacity);
	Serial.print("chargeFlag:"); Serial.println(values.chargeFlag);
	Serial.print("dischargeFlag:"); Serial.println(values.dischargeFlag);

	for (int probe = 0; probe < 6; probe++) {
		Serial.printf("Probe %d: %d", probe, values.temperatures[probe]);
	}

	for (int cell = 0; cell < BATTERY_CELL_COUNT; cell++) {
		Serial.print("Cell "); Serial.print(cell + 1); Serial.print(" : "); Serial.println(values.voltages[cell], 3);
	}
	Serial.print("\n\n");
}

void writeBlockBT(const uint8_t buf[], int size)
{
  for (int i=0; i<size; ++i)
    bluetooth::SerialBT.write(buf[i]);
  bluetooth::SerialBT.flush();
}

int readBlockBT(uint8_t buffer[], int max_size)
{
  int size = 0;
  while (bluetooth::SerialBT.available()) 
  {
    buffer[size] = bluetooth::SerialBT.read();
	++size;
	if (size > max_size)
		return false;
    delay(1);
  }
  return size;
}

int AntBMS::requestData(uint8_t buffer[], int size)
{
	if (!bluetooth::connected)
		return -1;
		
	while (bluetooth::SerialBT.available())
		bluetooth::SerialBT.read();

	writeBlockBT(REQUEST_DATA, 6);
	bluetooth::SerialBT.flush();
	while (!bluetooth::SerialBT.available())
		empty_statement;

  	int bytesRead = readBlockBT(buffer, size);
  	Serial.printf("Received %d bytes.\n", bytesRead);

	/*  if (i == 140) 
		Serial.printf("0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x\n", buffer[0], buffer[1],buffer[2],buffer[3],buffer[4], buffer[5]);
	Serial.printf("SOC %d\n", buffer[74]);*/

	return bytesRead;
}

bool AntBMS::isChecksumValid(uint8_t buffer[], int size)
{
	if (buffer_get_int32(buffer, 0) != 0xAA55AAFF || size != 140)
		return -1;

	uint16_t expected = 0;
	for (int i=3; i<137; ++i) 
		expected += buffer[i];

	uint16_t checksum = buffer_get_int16(buffer, 137);

	return checksum == expected;
}

tristate AntBMS::toggleAutoBalance()
{
	uint8_t data[] = {0xA5, 0xA5, 252, 0, 0, 252};

	while (bluetooth::SerialBT.available())
		bluetooth::SerialBT.read();
	writeBlockBT(data, 6);
	while (!bluetooth::SerialBT.available())
		empty_statement;

  	int bytesRead = readBlockBT(data, 6);
	if (bytesRead != 6)
		return 0;
	else if (data[3] != 0 || data[4] != 0)
		return 1;
	else 
		return -1;
}

tristate AntBMS::readAutoBalance()
{
	uint8_t data[] = {0x5A, 0x5A, 252, 0, 0, 252};

	while (bluetooth::SerialBT.available())
		bluetooth::SerialBT.read();
	writeBlockBT(data, 6);
	while (!bluetooth::SerialBT.available())
		empty_statement;

  	int bytesRead = readBlockBT(data, 6);
	if (bytesRead != 6)
		return 0;
	else if (data[3] != 0 || data[4] != 0)
		return 1;
	else 
		return -1;
}

tristate AntBMS::setAutoBalance(bool on)
{
	return 0;
	/*
	    self:evaluateParameters()

    util:log("Balancer status was", self.v.BalancedStatusText and string.lower(self.v.BalancedStatusText) or self.v.BalancedStatusFlag)

    if not self.v.BalancedStatusText then
        util:log("xxxx error self.v.BalancedStatusText is nil")
        return
    end
    if on then
        if string.find(string.lower(self.v.BalancedStatusText), "on") then
            return -- already on
        else
            self:toggleAutoBalance()
        end
    else
        if string.find(string.lower(self.v.BalancedStatusText), "off") then
            return -- already off
        else
            self:toggleAutoBalance()
        end
    end
*/
}

void AntBMS::reboot()
{
	uint8_t data[] = {0xA5, 0xA5, 254, 0, 0, 254};

	while (bluetooth::SerialBT.available())
		bluetooth::SerialBT.read();
	writeBlockBT(data, 6);
	while (!bluetooth::SerialBT.available())
		empty_statement;

  	int bytesRead = readBlockBT(data, 6);
}