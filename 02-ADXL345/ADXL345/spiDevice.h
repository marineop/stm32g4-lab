#ifndef SPIDEVICE_H_
#define SPIDEVICE_H_

#include "stm32g4xx_hal.h"
#include "FreeRTOS.h"
#include "semphr.h"

typedef struct
{
	SPI_HandleTypeDef* hspi;
	SemaphoreHandle_t mutex;
	SemaphoreHandle_t transferComplete;
} SpiDevice;

typedef enum
{
	SpiDiviceResult_Success,
	SpiDiviceResult_Unknown,
	SpiDiviceResult_HalError,
	SpiDiviceResult_MutexTimeout,
	SpiDiviceResult_CompleteTimeout,
} SpiDiviceResult;

void SpiDivice_Initialize(SpiDevice* spiDivice);

int32_t SpiDevice_Transfer(
	SpiDevice* spiDevice,
	GPIO_TypeDef* ssGpio,
	uint16_t ssPin,
	uint8_t* txData,
	uint8_t* rxData,
	int32_t length,
	TickType_t mutexTimeout,
	TickType_t completeTimeout);

#endif /* SPIDEVICE_H_ */
