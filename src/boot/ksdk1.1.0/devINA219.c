#include <stdlib.h>

/*
 *	config.h needs to come first
 */
#include "config.h"

#include "fsl_misc_utilities.h"
#include "fsl_device_registers.h"
#include "fsl_i2c_master_driver.h"
#include "fsl_spi_master_driver.h"
#include "fsl_rtc_driver.h"
#include "fsl_clock_manager.h"
#include "fsl_power_manager.h"
#include "fsl_mcglite_hal.h"
#include "fsl_port_hal.h"

#include "gpio_pins.h"
#include "SEGGER_RTT.h"
#include "warp.h"


extern volatile WarpI2CDeviceState	deviceINA219State;
extern volatile uint32_t		gWarpI2cBaudRateKbps;
extern volatile uint32_t		gWarpI2cTimeoutMilliseconds;
extern volatile uint32_t		gWarpSupplySettlingDelayMilliseconds;



void
initINA219(const uint8_t i2cAddress)
{
	deviceINA219State.i2cAddress			= i2cAddress;

	return;
}

WarpStatus
writeSensorRegisterINA219(uint8_t deviceRegister, uint16_t payload)
{
	uint8_t		payloadBytes[2], commandByte[1];
	i2c_status_t	returnValue;

	switch (deviceRegister)
	{
		case 0x00: case 0x01: case 0x02: case 0x03:
		case 0x04: case 0x05:
		{
			/* OK */
			break;
		}
		
		default:
		{
			return kWarpStatusBadDeviceCommand;
		}
	}

	i2c_device_t slave =
	{
		.address = deviceINA219State.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};

	commandByte[0] = deviceRegister;

	payloadBytes[0]= payload & 0xff;
	payloadBytes[1]=(payload >> 8);


	payloadBytes[0] = payload;
	warpEnableI2Cpins();

	returnValue = I2C_DRV_MasterSendDataBlocking(
							0 /* I2C instance */,
							&slave,
							commandByte,
							1,
							payloadBytes,
							2,
							gWarpI2cTimeoutMilliseconds);
	if (returnValue != kStatus_I2C_Success)
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}

WarpStatus
readSensorRegisterINA219(uint8_t deviceRegister, int numberOfBytes)
{
	uint8_t		cmdBuf[1]	= {0xFF};
	i2c_status_t	status1, status2;


	USED(numberOfBytes);
	i2c_device_t slave =
	{
		.address = deviceINA219State.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};

	/*
	 *	Steps (Repeated single-byte read. See Section 4.2.2 of MAG3110 manual.):
	 *
	 *	(1) Write transaction beginning with start condition, slave address, and pointer address.
	 *
	 *	(2) Read transaction beginning with start condition, followed by slave address, and read 1 byte payload
	*/

	cmdBuf[0] = deviceRegister;
	warpEnableI2Cpins();

	status1 = I2C_DRV_MasterSendDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							NULL,
							0,
							gWarpI2cTimeoutMilliseconds);

	status2 = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							(uint8_t *)deviceINA219State.i2cBuffer,
							numberOfBytes,
							gWarpI2cTimeoutMilliseconds);

	if ((status1 != kStatus_I2C_Success) || (status2 != kStatus_I2C_Success))
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}

void
printSensorDataINA219(bool hexModeFlag)
{
	uint16_t	readSensorRegisterValueLSB;
	uint16_t	readSensorRegisterValueMSB;
	int16_t		readSensorRegisterValueCombined;
	// int8_t		readSensorRegisterSignedByte;
	WarpStatus	i2cReadStatus;


	for (int i = 0; i < 5; i++){
		i2cReadStatus = readSensorRegisterINA219(i, 2 /* numberOfBytes */);
		readSensorRegisterValueMSB = deviceINA219State.i2cBuffer[0];
		readSensorRegisterValueLSB = deviceINA219State.i2cBuffer[1];
		readSensorRegisterValueCombined = ((readSensorRegisterValueMSB & 0xFF) << 8) | (readSensorRegisterValueLSB & 0xFF);

		/*
		*	NOTE: Here, we don't need to manually sign extend since we are packing directly into an int16_t
		*/

		if (i2cReadStatus != kWarpStatusOK)
		{
			warpPrint(" ----,");
		}
		else
		{
			if (hexModeFlag)
			{
				warpPrint(" 0x%02x 0x%02x,", readSensorRegisterValueMSB, readSensorRegisterValueLSB);
			}
			else
			{
				warpPrint(" %d,", readSensorRegisterValueCombined/20);
			}
		}
	}
}

void
printCurrentDataINA219(bool hexModeFlag)
{
	uint16_t	readSensorRegisterValueLSB;
	uint16_t	readSensorRegisterValueMSB;
	int16_t		readSensorRegisterValueCombined;
	// int8_t		readSensorRegisterSignedByte;
	WarpStatus	i2cReadStatus;


	for (int i = 0; i < 5; i++){
		i2cReadStatus = readSensorRegisterINA219(kWarpSensorOutputRegisterINA219_CURRENT_MSB, 2 /* numberOfBytes */);
		readSensorRegisterValueMSB = deviceINA219State.i2cBuffer[0];
		readSensorRegisterValueLSB = deviceINA219State.i2cBuffer[1];
		readSensorRegisterValueCombined = ((readSensorRegisterValueMSB & 0xFF) << 8) | (readSensorRegisterValueLSB & 0xFF);

		/*
		*	NOTE: Here, we don't need to manually sign extend since we are packing directly into an int16_t
		*/

		if (i2cReadStatus != kWarpStatusOK)
		{
			warpPrint(" ----,");
		}
		else
		{
			if (hexModeFlag)
			{
				warpPrint(" 0x%02x 0x%02x,", readSensorRegisterValueMSB, readSensorRegisterValueLSB);
			}
			else
			{
				warpPrint(" %d \n,", readSensorRegisterValueCombined);
			}
		}
	}
}