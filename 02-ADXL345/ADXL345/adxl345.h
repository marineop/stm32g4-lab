#ifndef ADXL345_H_
#define ADXL345_H_

#include "stm32g4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"
#include "spiDevice.h"

typedef struct
{
	char* taskName;
	uint32_t stackDepth;
	UBaseType_t taskPriority;
	StackType_t* stackBuffer;

	size_t messagesSize;
	uint8_t* messagesStorageArea;

	SpiDevice* spi;
	GPIO_TypeDef* ssGpio;
	uint16_t ssPin;
} ADXL345Settings;

typedef struct
{
	TaskHandle_t task;
	StaticTask_t taskStruct;

	MessageBufferHandle_t messages;
	StaticMessageBuffer_t messagesStruct;

	SpiDevice* spi;
	GPIO_TypeDef* ssGpio;
	uint16_t ssPin;
	uint8_t txBuffer[16];
	uint8_t rxBuffer[16];

	uint8_t intMap;
} ADXL345;

typedef struct
{
	int16_t x;
	int16_t y;
	int16_t z;
} AccData;

typedef enum
{
	ADXL345TapAxes_X = 0x04,
	ADXL345TapAxes_Y = 0x02,
	ADXL345TapAxes_Z = 0x01,
} ADXL345TapAxes;

typedef enum
{
	ADXL345IntEnable_DataReady = 0x80,
	ADXL345IntEnable_SingleTap = 0x40,
	ADXL345IntEnable_DoubleTap = 0x20,
	ADXL345IntEnable_Activity = 0x10,
	ADXL345IntEnable_Inactivity = 0x08,
	ADXL345IntEnable_FreeFall = 0x04,
	ADXL345IntEnable_Watermark = 0x02,
	ADXL345IntEnable_Overrun = 0x01,
} ADXL345IntEnable;

typedef enum
{
	ADXL345Range_2,
	ADXL345Range_4,
	ADXL345Range_8,
	ADXL345Range_16,
} ADXL345Range;

typedef enum
{
	ADXL345OutputRate_3200 = 0xF,
	ADXL345OutputRate_1600 = 0xE,
	ADXL345OutputRate_800 = 0xD,
	ADXL345OutputRate_400 = 0xC,
	ADXL345OutputRate_200 = 0xB,
	ADXL345OutputRate_100 = 0xA,
	ADXL345OutputRate_50 = 0x9,
	ADXL345OutputRate_25 = 0x8,
	ADXL345OutputRate_12_5 = 0x7,
	ADXL345OutputRate_6_25 = 0x6,
	ADXL345OutputRate_3_13 = 0x5,
	ADXL345OutputRate_1_56 = 0x4,
	ADXL345OutputRate_0_78 = 0x3,
	ADXL345OutputRate_0_39 = 0x2,
	ADXL345OutputRate_0_20 = 0x1,
	ADXL345OutputRate_0_10 = 0x0,

} ADXL345OutputRate;

typedef enum
{
	ADXL345IntMap_DataReadyInt1 = 0x00,
	ADXL345IntMap_DataReadyInt2 = 0x80,
	ADXL345IntMap_SingleTapInt1 = 0x00,
	ADXL345IntMap_SingleTapInt2 = 0x40,
	ADXL345IntMap_DoubleTapInt1 = 0x00,
	ADXL345IntMap_DoubleTapInt2 = 0x20,
	ADXL345IntMap_ActivityInt1 = 0x00,
	ADXL345IntMap_ActivityInt2 = 0x10,
	ADXL345IntMap_InactivityInt1 = 0x00,
	ADXL345IntMap_InactivityInt2 = 0x08,
	ADXL345IntMap_FreeFallInt1 = 0x00,
	ADXL345IntMap_FreeFallInt2 = 0x04,
	ADXL345IntMap_WatermarkInt1 = 0x00,
	ADXL345IntMap_WatermarkInt2 = 0x02,
	ADXL345IntMap_OverrunInt1 = 0x00,
	ADXL345IntMap_OverrunInt2 = 0x01,
} ADXL345IntMap;

typedef enum
{
	ADXL345Event_Data = 0x00,
	ADXL345Event_Interrupt = 0x01,
} ADXL345Event;

void ADXL345Settings_Initialize(ADXL345Settings* settings);

void ADXL345_Initialize(ADXL345* acc, ADXL345Settings* settings);

void ADXL345_INT1ISR(ADXL345* acc);

void ADXL345_INT2ISR(ADXL345* acc);

int32_t ADXL345_ReadDevid(ADXL345* acc, uint8_t* devid);

// 62.5 mg/LSB
int32_t ADXL345_WriteThreshTap(ADXL345* acc, uint8_t threshold);

// 625 µs/LSB
int32_t ADXL345_WriteDur(ADXL345* acc, uint8_t duration);

int32_t ADXL345_WriteTapAxes(ADXL345* acc, uint8_t axes);

int32_t ADXL345_WriteIntEnable(ADXL345* acc, uint8_t intEnableFlags);

int32_t ADXL345_ReadIntSource(ADXL345* acc, uint8_t* intSource);

int32_t ADXL345_WriteFullRes(ADXL345* acc, uint8_t fullRes);

int32_t ADXL345_WriteRange(ADXL345* acc, ADXL345Range range);

int32_t ADXL345_WriteOutputRate(ADXL345* acc, ADXL345OutputRate outputRate);

int32_t ADXL345_WriteIntMap(ADXL345* acc, uint8_t intMap);

int32_t ADXL345_WriteMeasure(ADXL345* acc, uint8_t measure);

#endif /* ADXL345_H_ */
