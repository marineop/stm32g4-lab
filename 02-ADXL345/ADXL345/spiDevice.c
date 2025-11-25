#include "spiDevice.h"

SpiDevice* spiDevices[2] = {0};
int32_t spiDevicesCount = 0;

SpiDevice* GetSpiDevice(SPI_HandleTypeDef* hspi)
{
	for (int32_t i = 0; i < spiDevicesCount; ++i)
	{
		if (spiDevices[i]->hspi == hspi)
		{
			return spiDevices[i];
		}
	}

	configASSERT(0 && "unknown SPI device");

	return NULL;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi)
{
	SpiDevice* spiDevice = GetSpiDevice(hspi);
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	xSemaphoreGiveFromISR(spiDevice->transferComplete, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void SpiDivice_Initialize(SpiDevice* spiDevice)
{
	spiDevice->mutex = xSemaphoreCreateMutex();
	configASSERT(spiDevice->mutex);

	spiDevice->transferComplete = xSemaphoreCreateBinary();

	spiDevices[spiDevicesCount++] = spiDevice;
}

int32_t SpiDevice_Transfer(
	SpiDevice* spiDevice,
	GPIO_TypeDef* ssGpio,
	uint16_t ssPin,
	uint8_t* txData,
	uint8_t* rxData,
	int32_t length,
	TickType_t mutexTimeout,
	TickType_t completeTimeout)
{
	int32_t result = SpiDiviceResult_Unknown;

	configASSERT(spiDevice->mutex);
	if (xSemaphoreTake(spiDevice->mutex, mutexTimeout) == pdTRUE)
	{
		HAL_GPIO_WritePin(ssGpio, ssPin, GPIO_PIN_RESET);
		HAL_StatusTypeDef halResult = HAL_SPI_TransmitReceive_DMA(spiDevice->hspi, txData, rxData, length);

		if (halResult != HAL_OK)
		{
			result = SpiDiviceResult_HalError;
		}
		else
		{
			if (xSemaphoreTake(spiDevice->transferComplete, completeTimeout) == pdTRUE)
			{
				result = SpiDiviceResult_Success;
			}
			else
			{
				result = SpiDiviceResult_CompleteTimeout;
				configASSERT(0 && "xSemaphoreTake timeout");
			}
		}

		HAL_GPIO_WritePin(ssGpio, ssPin, GPIO_PIN_SET);

		xSemaphoreGive(spiDevice->mutex);
	}
	else
	{
		result = SpiDiviceResult_MutexTimeout;
		configASSERT(0 && "xSemaphoreTake failed");
	}

	return result;
}
