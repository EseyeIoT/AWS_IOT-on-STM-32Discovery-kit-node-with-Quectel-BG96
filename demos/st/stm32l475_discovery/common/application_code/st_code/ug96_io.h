/**
  ******************************************************************************
  * @file    ug96_io.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the C2C IO operations.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UG96_IO__
#define __UG96_IO__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Exported constants --------------------------------------------------------*/
/* This section can be used to tailor UART_C2C instance used and associated
   resources */
#define UART_C2C                           UART4
#define UART_C2C_CLK_ENABLE()              __HAL_RCC_UART4_CLK_ENABLE();
#define DMAx_CLK_ENABLE()                   __HAL_RCC_DMA2_CLK_ENABLE()
#define UART_C2C_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define UART_C2C_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()

#define UART_C2C_FORCE_RESET()             __HAL_RCC_UART4_FORCE_RESET()
#define UART_C2C_RELEASE_RESET()           __HAL_RCC_UART4_RELEASE_RESET()

/* Definition for UART_C2C Pins */
#define UART_C2C_TX_PIN                    GPIO_PIN_0
#define UART_C2C_TX_GPIO_PORT              GPIOA
#define UART_C2C_TX_AF                     GPIO_AF8_UART4


/* PortG on L696AG device needs Independent I/O supply rail;
   It can be enabled by setting the IOSV bit in the PWR_CR2 register, 
   when the VDDIO2 supply is present (depends by the package).*/
#define UART_C2C_RX_PIN                    GPIO_PIN_1
#define UART_C2C_RX_GPIO_PORT              GPIOA
#define UART_C2C_RX_AF                     GPIO_AF8_UART4

#define UART_C2C_RTS_PIN                   GPIO_PIN_2
#define UART_C2C_RTS_GPIO_PORT             GPIOA
//#define UART_C2C_RTS_AF                    GPIO_AF7_USART1

#define UART_C2C_CTS_PIN                   GPIO_PIN_14
#define UART_C2C_CTS_GPIO_PORT             GPIOD
//#define UART_C2C_CTS_AF                    GPIO_AF7_USART1
   
/* Definition for UART_C2C's NVIC IRQ and IRQ Handlers */
#define UART_C2C_IRQn                      UART4_IRQn
//#define UART_C2C_IRQHandler                USART1_IRQHandler

/* Definition for UART_C2C's DMA */
#define UART_C2C_TX_DMA_CHANNEL            DMA2_Channel3
/* Definition for UART_C2C's DMA Request */
#define UART_C2C_TX_DMA_REQUEST            DMA_REQUEST_2
/* Definition for UART_C2C's NVIC */
#define UART_C2C_DMA_TX_IRQn               DMA2_Channel3_IRQn
//#define UART_C2C_DMA_TX_IRQHandler         DMA1_Channel4_IRQHandler

/* C2C module Reset pin definitions */
//#define C2C_RST_PIN                        GPIO_PIN_2
//#define C2C_RST_GPIO_PORT                  GPIOC
//#define C2C_RST_GPIO_CLK_ENABLE()          __HAL_RCC_GPIOC_CLK_ENABLE()

/* C2C module PowerKey pin definitions */
#define C2C_PWRKEY_PIN                     GPIO_PIN_2
#define C2C_PWRKEY_GPIO_PORT               GPIOC
#define C2C_PWRKEY_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOC_CLK_ENABLE()

/* Modem status pin */
#define C2C_STATUS_PIN                GPIO_PIN_5
#define C2C_STATUS_GPIO_PORT          GPIOC
#define C2C_STATUS_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOC_CLK_ENABLE()

int8_t  UART_C2C_Init(void);
int8_t  UART_C2C_DeInit(void);
int8_t  UART_C2C_SetBaudrate(uint32_t BaudRate);
void    UART_C2C_FlushBuffer(void);
int16_t UART_C2C_SendData(uint8_t* Buffer, uint16_t Length);
int16_t UART_C2C_ReceiveSingleData(uint8_t* pData);

#ifdef __cplusplus
}
#endif

#endif /* __UG96_IO__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
