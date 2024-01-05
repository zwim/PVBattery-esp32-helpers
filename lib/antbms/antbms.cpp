
#include "Arduino.h"

#include "antbms.h"
#include "bluetooth.h"

const uint8_t REQUEST_DATA[] = {0xDB, 0xDB, 0x00, 0x00, 0x00, 0x00};

uint16_t buffer_get_uint16(const uint8_t buffer[], uint8_t index) {
	return (((uint16_t) buffer[index]) << 8) | ((uint16_t) buffer[index + 1]);
}
int16_t buffer_get_int16(const uint8_t buffer[], uint8_t index) {
	return (int16_t) buffer_get_uint16(buffer, index);
}

uint32_t buffer_get_uint32(const uint8_t buffer[], uint8_t index) {
	return ((uint32_t) buffer[index]) << 24 | ((uint32_t) buffer[index + 1]) << 16 |
		((uint32_t) buffer[index + 2]) << 8 | ((uint32_t) buffer[index + 3]);
}
int32_t buffer_get_int32(const uint8_t buffer[], uint8_t index) {
	return (int32_t) buffer_get_uint32(buffer, index);
}

AntBMS bms;

bool AntBMS::processPacket() 
{
	if (!isLiveDataChecksumValid())
	{
		Serial.println("ANT-BMS wrong checksum.");
		return false;
	}

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
	values.relaySwitch = buffer[110];
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

	lastDataMillis = millis();

	return true;
}

void AntBMS::printSerial()
{
	Serial.print("Current:"); Serial.println(values.current);
	Serial.print("Percentage:"); Serial.println(values.percentage);
	Serial.print("totalCapacity:"); Serial.println(values.totalCapacity);
	Serial.print("remainingCapacity:"); Serial.println(values.remainingCapacity);
	Serial.print("totalCapacity:"); Serial.println(values.totalCapacity);
	Serial.print("chargeFlag: 0x"); Serial.println(values.chargeFlag);
	Serial.print("dischargeFlag: 0x"); Serial.println(values.dischargeFlag);

	for (int probe = 0; probe < 6; ++probe) 
		Serial.printf("Probe %d: %d", probe, values.temperatures[probe]);
	Serial.println("\n");

	for (int cell = 0; cell < values.numberOfBatteries; ++cell) 
	{
		Serial.printf("Cell % 2d: % 2.3f", cell, values.voltages[cell]);
		Serial.println();
	}
	Serial.print("\n\n");
}

int AntBMS::requestData()
{
	if (!bluetooth::connected)
		return -1;

	if (dataAge() < 1000)
		return sizeof(buffer);
		
	while (bluetooth::SerialBT.available())
		bluetooth::SerialBT.read();

	bluetooth::writeBlockBT(REQUEST_DATA, sizeof(REQUEST_DATA));
	bluetooth::SerialBT.flush();

	uint old_time = millis();
	while (!bluetooth::SerialBT.available() && (millis() - old_time < 1000))
		empty_statement;

  	int bytesRead = bluetooth::readBlockBT(buffer, sizeof(buffer));
	return bytesRead;
}

bool AntBMS::isLiveDataChecksumValid()
{
	uint32_t header = buffer_get_int32(buffer, 0);
	if (header != 0xAA55AAFF)
		return false;

	uint16_t expected = 0;
	for (int i=4; i<138; ++i) 
		expected += buffer[i];

	uint16_t checksum = buffer_get_uint16(buffer, 138);

	if (checksum == expected)
		Serial.printf("checksum OK\n");
	else
		Serial.printf("ERROR checksum: %d, exp: %d\n", checksum, expected);

	return (checksum == expected);
}

bool AntBMS::isFrameChecksumValid(const uint8_t buffer[])
{
	uint32_t header = buffer_get_uint16(buffer, 0);
	if ( header != 0xa5a5 && header != 0x5a5a && header != 0xdbdb)
		return false;

	for (int i=0; i<6; ++i)
		Serial.printf("0x%x ", buffer[i]);

	uint8_t expected = 0;
	for (int i=2; i<4; ++i) 
		expected += buffer[i];
 
	uint8_t checksum = buffer[5];

	if (checksum == expected)
		Serial.printf("checksum OK (%d)\n", sizeof(buffer));
	else
		Serial.printf("ERROR checksum: %d, exp: %d\n", checksum, expected);

	return (checksum == expected);
}

tristate AntBMS::toggleAutoBalance()
{   // toggleAutoBalance
	uint8_t data[] = {0xA5, 0xA5, 252, 0, 0, 252};

	while (bluetooth::SerialBT.available())
		bluetooth::SerialBT.read();
	bluetooth::writeBlockBT(data, sizeof(data));
	bluetooth::SerialBT.flush();

	uint old_time = millis();
	while (!bluetooth::SerialBT.available() && (millis() - old_time < 1000))
		empty_statement;

  	int bytesRead = bluetooth::readBlockBT(data, sizeof(data));

	wait(1000);
	return readAutoBalance();
}

/*
	This does not work as expected, maybe a bug in the antbms?
tristate AntBMS::readAutoBalance()
{  // readAutoBalance
	uint8_t data[] = {0x5A, 0x5A, 252, 0, 0, 252};

	while (bluetooth::SerialBT.available())
		bluetooth::SerialBT.read();
	bluetooth::writeBlockBT(data, sizeof(data));
	bluetooth::SerialBT.flush();

	uint old_time = millis();
	while (!bluetooth::SerialBT.available() && (millis() - old_time < 1000))
		empty_statement;

	// fill array with stupid data (wrong CRC)
	for (int i = 0; i < 6; ++i)
		data[i] = i;

  	int bytesRead = bluetooth::readBlockBT(data, sizeof(data));
	if (bytesRead != 6 || !isFrameChecksumValid(data))
		return 0;
	else if (data[3] != 0 || data[4] != 0)
		return 1;
	else 
		return -1;
}
*/

tristate AntBMS::readAutoBalance()
{
    if (!requestAndProcess())
		return 0;

	if (values.balancedStatusFlag == 0)
		return -1; // off
	else 
		return 1; // on
}


tristate AntBMS::setAutoBalance(bool on)
{
	tristate current_state = readAutoBalance();
	if ((on && current_state == 1) || (!on && current_state != 1))
		return current_state;
	else
	{
		toggleAutoBalance();
		wait(1000);
		return readAutoBalance();
	}
}

void AntBMS::reboot()
{
	uint8_t data[] = {0xA5, 0xA5, 254, 0, 0, 254};

	while (bluetooth::SerialBT.available())
		bluetooth::SerialBT.read();
	bluetooth::writeBlockBT(data, sizeof(data));
	bluetooth::SerialBT.flush();

	uint old_time = millis();
	while (!bluetooth::SerialBT.available() && (millis() - old_time < 1000))
		empty_statement;

  	bluetooth::readBlockBT(data, sizeof(data));
}

bool AntBMS::setDischargeMos(uint8_t state)
{
    if (!requestAndProcess())
		return false;

	if (state == 0 && values.dischargeFlag == 0xf)
		return true; // already off
	else if (state == 1 && values.dischargeFlag == 0x1)
		return true; // already on
	
	// toggle discharge MOSFET
	uint8_t data[] = {0xA5, 0xA5, 249, 0, state, (uint8_t) (249 + state)};

	while (bluetooth::SerialBT.available())
		bluetooth::SerialBT.read();
	bluetooth::writeBlockBT(data, sizeof(data));
	bluetooth::SerialBT.flush();

	uint old_time = millis();
	while (!bluetooth::SerialBT.available() && (millis() - old_time < 1000))
		empty_statement;

  	bluetooth::readBlockBT(data, sizeof(data));

	return true;
}

// reading ANT-BMS data and process them
bool AntBMS::requestAndProcess()
{
	if (dataAge() < 1000)
		return true;
    int bytesRead = requestData();
	Serial.printf("Bytes read: %d\n", bytesRead);
    if (bytesRead > 0 )
    {
        if (processPacket()) 
		{
        	printSerial();
			return true;
		}
		else
			Serial.println("Error processing BT-Packet.");
    }
	return false;
}

