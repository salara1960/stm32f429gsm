/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <stdarg.h>
#include <ctype.h>

#include "hdr.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
enum {
	devOK = 0,
	devTIK = 1,
	devUART = 2,
	devMEM = 4,
	devRTC = 8,
	devSPI = 0x10,
	devVCP = 0x20,
	devQue = 0x40,
	devFS  = 0x80,
	devModem = 0x100
};

enum {
	sNone = -1,
	sRead,
	sWrite,
	sErase,
	sNext,
	sModCmd,
	sModAck,
	sModPwrOn,
	sModPwrOff,
	sModRst,
	sGetMsg,
	sFill,
	sRdDir,
	sRdFile,
	sMkDir,
	sRemove,
	sRestart
};
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

#define VERSION "Ver.0.1.6 from 06.08.24 09:18:00"

#define MAX_UART_BUF  2048
#define MAX_SEC_PRN     64
#define MAX_ADDR_LEN   128
//#define MAX_QREC        16//64//16
#define MAX_SQREC       16//64//16

#define _250ms   1
#define _500ms   2
#define _750ms   3
#define _1s      4
#define _1s25    5
#define _1s5     6
#define _1s75    7
#define _2s      _1s * 2
#define _2s25    _2s + _250ms
#define _2s5     _2s + _500ms
#define _2s75    _2s + _750ms
#define _3s      _1s * 3

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

#pragma pack(push,1)
typedef struct q_rec_t {
	int8_t id;
	char *adr;
} q_rec_t;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct s_recq_t {
	volatile uint8_t lock;
	uint8_t put;
	uint8_t get;
	q_rec_t rec[MAX_SQREC];
} s_recq_t;
#pragma pack(pop)

//const int FR_ERR = -1;

extern const char *eol;
//extern SPI_HandleTypeDef *portFLASH;
extern uint8_t spiRdy;
extern uint16_t devError;
extern bool chipPresent;

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

void Report(const char *tag, bool addTime, const char *fmt, ...);


#ifdef SET_W25FLASH
	#include "w25.h"

	extern uint8_t w25_withDMA;

	extern void W25qxx_Reset();
	extern void W25_SELECT();
	extern void W25_UNSELECT();
	extern bool W25qxx_Init(void);
	extern uint32_t W25qxx_getChipID();
#endif

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SPI4_RST_Pin GPIO_PIN_3
#define SPI4_RST_GPIO_Port GPIOE
#define MOD_WAKE_Pin GPIO_PIN_5
#define MOD_WAKE_GPIO_Port GPIOA
#define MOD_RST_Pin GPIO_PIN_6
#define MOD_RST_GPIO_Port GPIOA
#define MOD_PWR_ON_Pin GPIO_PIN_10
#define MOD_PWR_ON_GPIO_Port GPIOE
#define LED_MODE_Pin GPIO_PIN_13
#define LED_MODE_GPIO_Port GPIOE
#define SPI4_CS1_Pin GPIO_PIN_9
#define SPI4_CS1_GPIO_Port GPIOB
#define SPI4_HOLD_Pin GPIO_PIN_0
#define SPI4_HOLD_GPIO_Port GPIOE
#define SPI4_WP_Pin GPIO_PIN_1
#define SPI4_WP_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

#define LED_TIK_ON()

#ifdef SET_W25FLASH

	extern SPI_HandleTypeDef *portFLASH;
	extern uint32_t spi_cnt;
	extern uint8_t spiRdy;

	#define W25_CS_GPIO_Port SPI4_CS1_GPIO_Port
	#define W25_CS_Pin SPI4_CS1_Pin

	#define W25_RST_GPIO_Port SPI4_RST_GPIO_Port
	#define W25_RST_Pin SPI4_RST_Pin

#endif



#define LOOP_FOREVER() while(1) { HAL_Delay(1); }

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
