#include "adxl345.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// command
#define ADDRESS_MASK	(0x3F)
#define READ			(0x80)
#define WRITE			(0x00)
#define MULTI_BYTE		(0x40)
#define SINGLE_BYTE		(0x00)

// registers
#define DEVID			(0x00)

#define THRESH_TAP		(0x1D)

#define DUR				(0x21)

#define TAP_AXES		(0x2A)
#define TAP_AXES_XYZ			(0x07)

#define BW_RATE			(0x2C)
#define BW_RATE_RATE			(0x0F)

#define POWER_CTRL		(0x2D)
#define POWER_CTRL_MEASURE		(0x08)

#define INT_ENABLE		(0x2E)
#define INT_ENABLE_DATA_READY	(0x80)

#define INT_MAP			(0x2F)
#define INT_MAP_DATA_READY		(0x80)

#define INT_SOURCE		(0x30)
#define NON_DATA_INT	(ADXL345IntEnable_SingleTap\
							| ADXL345IntEnable_DoubleTap \
							| ADXL345IntEnable_Activity \
							| ADXL345IntEnable_Inactivity\
							| ADXL345IntEnable_FreeFall \
							| ADXL345IntEnable_Overrun)

#define DATA_FORMAT		(0x31)
#define DATA_FORMAT_RANGE		(0x03)
#define DATA_FORMAT_FULL_RES	(0x08)

#define DATAX0			(0x32)

#define FIFO_CTL		(0x38)
#define FIFO_STATUS		(0x39)

// interrupt notify
#define NOTIFY_INT1		(0x01)
#define NOTIFY_INT2		(0x02)

#define SET_BIT_VALUE(TARGET, MASK, VALUE) (VALUE)?((TARGET) |= (MASK)):((TARGET) &= ~(MASK))

#define SET_BITS_VALUE(TARGET, MASK, VALUE) \
	do \
	{ \
		(TARGET) &= ~(MASK); \
		(TARGET) |= (VALUE) & (MASK); \
	} while(0)

static int32_t WriteRegister(ADXL345* acc, uint8_t address, uint8_t value)
{
	memset(acc->txBuffer, 0, 2);
	acc->txBuffer[0] = WRITE | SINGLE_BYTE | (address & ADDRESS_MASK);
	acc->txBuffer[1] = value;
	memset(acc->rxBuffer, 0, 2);
	return SpiDevice_Transfer(
		acc->spi,
		acc->ssGpio, acc->ssPin,
		acc->txBuffer, acc->rxBuffer, 2,
		pdMS_TO_TICKS(1000),
		pdMS_TO_TICKS(10));
}

static int32_t WriteRegisters(ADXL345* acc, uint8_t address, int32_t length)
{
	acc->txBuffer[0] = WRITE | MULTI_BYTE | (address & ADDRESS_MASK);
	return SpiDevice_Transfer(
		acc->spi,
		acc->ssGpio, acc->ssPin,
		acc->txBuffer, acc->rxBuffer, 1 + length,
		pdMS_TO_TICKS(1000),
		pdMS_TO_TICKS(10));
}

static int32_t ReadRegister(ADXL345* acc, uint8_t address)
{
	memset(acc->txBuffer, 0, 2);
	acc->txBuffer[0] = READ | SINGLE_BYTE | (address & ADDRESS_MASK);
	memset(acc->rxBuffer, 0, 2);
	return SpiDevice_Transfer(
		acc->spi,
		acc->ssGpio, acc->ssPin,
		acc->txBuffer, acc->rxBuffer, 2,
		pdMS_TO_TICKS(1000),
		pdMS_TO_TICKS(10));
}

static int32_t ReadRegisters(ADXL345* acc, uint8_t address, int32_t length)
{
	memset(acc->txBuffer, 0, 1 + length);
	acc->txBuffer[0] = READ | MULTI_BYTE | (address & ADDRESS_MASK);
	memset(acc->rxBuffer, 0, 1 + length);
	return SpiDevice_Transfer(
		acc->spi,
		acc->ssGpio, acc->ssPin,
		acc->txBuffer, acc->rxBuffer, 1 + length,
		pdMS_TO_TICKS(1000),
		pdMS_TO_TICKS(10));
}

static void taskLoop(void* pvParameters)
{
	ADXL345* acc = (ADXL345*)pvParameters;
	uint32_t notifyValue;

	while (1)
	{
		xTaskNotifyWait(
			0,
			0xFFFFFFFF,
			&notifyValue,
			pdMS_TO_TICKS(1000));

		if (((notifyValue & NOTIFY_INT1) && !(acc->intMap & INT_MAP_DATA_READY))
			|| ((notifyValue & NOTIFY_INT2) && (acc->intMap & INT_MAP_DATA_READY)))
		{
			ReadRegisters(acc, DATAX0, 6);

			uint8_t message[7];
			message[0] = ADXL345Event_Data;
			memcpy(message + 1, acc->rxBuffer + 1, 6);
			xMessageBufferSend(
				acc->messages,
				message,
				sizeof(message),
				pdMS_TO_TICKS(100));
		}

		if (notifyValue & (NOTIFY_INT1 | NOTIFY_INT2))
		{
			uint8_t intSource;
			ADXL345_ReadIntSource(acc, &intSource);
			if (intSource & NON_DATA_INT)
			{
				uint8_t message[2];
				message[0] = ADXL345Event_Interrupt;
				message[1] = intSource;
				xMessageBufferSend(
					acc->messages,
					message,
					sizeof(message),
					pdMS_TO_TICKS(100));
			}
		}
	}
}

void ADXL345Settings_Initialize(ADXL345Settings* settings)
{
	settings->taskName = "ADXL345";
	settings->stackDepth = 512;
	settings->taskPriority = 30;
	settings->stackBuffer = NULL;

	settings->messagesSize = 0;
	settings->messagesStorageArea = NULL;

	settings->spi = NULL;
	settings->ssGpio = NULL;
	settings->ssPin = GPIO_PIN_0;
}

void ADXL345_Initialize(ADXL345* acc, ADXL345Settings* settings)
{
	acc->task = xTaskCreateStatic(
		taskLoop,
		settings->taskName,
		settings->stackDepth,
		acc,
		settings->taskPriority,
		settings->stackBuffer,
		&acc->taskStruct);

	configASSERT(acc->task);

	acc->messages = xMessageBufferCreateStatic(
		settings->messagesSize,
		settings->messagesStorageArea,
		&acc->messagesStruct);

	configASSERT(acc->messages);

	acc->spi = settings->spi;
	acc->ssGpio = settings->ssGpio;
	acc->ssPin = settings->ssPin;

	int32_t result = ReadRegister(acc, INT_MAP);

	configASSERT(result == SpiDiviceResult_Success);

	acc->intMap = acc->txBuffer[1];
}

void ADXL345_INT1ISR(ADXL345* acc)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	xTaskNotifyFromISR(
		acc->task,
		NOTIFY_INT1,
		eSetBits,
		&xHigherPriorityTaskWoken);

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void ADXL345_INT2ISR(ADXL345* acc)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	xTaskNotifyFromISR(
		acc->task,
		NOTIFY_INT2,
		eSetBits,
		&xHigherPriorityTaskWoken);

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

int32_t ADXL345_ReadDevid(ADXL345* acc, uint8_t* devid)
{
	int32_t result = ReadRegister(acc, DEVID);
	if (result == SpiDiviceResult_Success)
	{
		*devid = acc->rxBuffer[1];
	}

	return result;
}

int32_t ADXL345_WriteThreshTap(ADXL345* acc, uint8_t threshold)
{
	return WriteRegister(acc, THRESH_TAP, threshold);
}

int32_t ADXL345_WriteDur(ADXL345* acc, uint8_t duration)
{
	return WriteRegister(acc, DUR, duration);
}

int32_t ADXL345_WriteTapAxes(ADXL345* acc, uint8_t axes)
{
	int32_t result = ReadRegister(acc, TAP_AXES);
	if (result == SpiDiviceResult_Success)
	{
		uint8_t tapAxes = acc->rxBuffer[1];
		SET_BITS_VALUE(tapAxes, TAP_AXES_XYZ, axes);
		result = WriteRegister(acc, TAP_AXES, tapAxes);
	}

	return result;
}

int32_t ADXL345_WriteIntEnable(ADXL345* acc, uint8_t intEnableFlags)
{
	return WriteRegister(acc, INT_ENABLE, intEnableFlags);
}

int32_t ADXL345_ReadIntSource(ADXL345* acc, uint8_t* intSource)
{
	int32_t result = ReadRegister(acc, INT_SOURCE);
	if (result == SpiDiviceResult_Success)
	{
		*intSource = acc->rxBuffer[1];
	}

	return result;
}

int32_t ADXL345_WriteFullRes(ADXL345* acc, uint8_t fullRes)
{
	int32_t result = ReadRegister(acc, DATA_FORMAT);
	if (result == SpiDiviceResult_Success)
	{
		uint8_t dataFormat = acc->rxBuffer[1];
		SET_BIT_VALUE(dataFormat, DATA_FORMAT_FULL_RES, fullRes);
		result = WriteRegister(acc, DATA_FORMAT, dataFormat);
	}

	return result;
}

int32_t ADXL345_WriteRange(ADXL345* acc, ADXL345Range range)
{
	int32_t result = ReadRegister(acc, DATA_FORMAT);
	if (result == SpiDiviceResult_Success)
	{
		uint8_t dataFormat = acc->rxBuffer[1];
		SET_BITS_VALUE(dataFormat, DATA_FORMAT_RANGE, range);
		result = WriteRegister(acc, DATA_FORMAT, dataFormat);
	}

	return result;
}

int32_t ADXL345_WriteOutputRate(ADXL345* acc, ADXL345OutputRate outputRate)
{
	return WriteRegister(acc, BW_RATE, outputRate);

	int32_t result = ReadRegister(acc, BW_RATE);
	if (result == SpiDiviceResult_Success)
	{
		uint8_t bwRate = acc->rxBuffer[1];
		SET_BITS_VALUE(bwRate, BW_RATE_RATE, outputRate);
		result = WriteRegister(acc, BW_RATE, bwRate);
	}

	return result;
}

int32_t ADXL345_WriteIntMap(ADXL345* acc, uint8_t intMap)
{
	int32_t result = WriteRegister(acc, INT_MAP, intMap);

	if (result == SpiDiviceResult_Success)
	{
		acc->intMap = intMap;
	}

	return result;
}

int32_t ADXL345_WriteMeasure(ADXL345* acc, uint8_t measure)
{
	int32_t result = ReadRegister(acc, POWER_CTRL);
	if (result == SpiDiviceResult_Success)
	{
		uint8_t powerControl = acc->rxBuffer[1];
		SET_BIT_VALUE(powerControl, POWER_CTRL_MEASURE, measure);
		result = WriteRegister(acc, POWER_CTRL, powerControl);
	}

	return result;
}

