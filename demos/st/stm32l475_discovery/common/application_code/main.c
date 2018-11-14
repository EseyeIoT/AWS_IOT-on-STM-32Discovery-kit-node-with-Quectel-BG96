/*
 * Amazon FreeRTOS V1.2.6
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */


/*
 * Debug setup instructions:
 * 1) Open the debug configuration dialog.
 * 2) Go to the Debugger tab.
 * 3) If the 'Mode Setup' options are not visible, click the 'Show Generator' button.
 * 4) In the Mode Setup|Reset Mode drop down ensure that
 *    'Software System Reset' is selected.
 */

#include "main.h"
#include "stdint.h"
#include "stdarg.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Demo includes */
#include "aws_demo_runner.h"
#include "aws_system_init.h"
#include "aws_logging_task.h"
#include "aws_wifi.h"
#include "aws_clientcredential.h"
#include "aws_dev_mode_key_provisioning.h"

/* The SPI driver polls at a high priority. The logging task's priority must also
 * be high to be not be starved of CPU time. */
#define mainLOGGING_TASK_PRIORITY           ( configMAX_PRIORITIES - 1 )
#define mainLOGGING_TASK_STACK_SIZE         ( configMINIMAL_STACK_SIZE * 5 )
#define mainLOGGING_MESSAGE_QUEUE_LENGTH    ( 15 )

void vApplicationDaemonTaskStartupHook( void );

/**********************
* Global Variables
**********************/
RTC_HandleTypeDef xHrtc;
RNG_HandleTypeDef xHrng;

#ifdef USE_ESEYE
#include "stm32l4xx_hal_tim.h"
#include "eseye.h"
#include "timers.h"
#include "ug96_io.h"
#include "ug96_conf.h"
#define RING_BUFFER_SIZE 2048

typedef struct
{
  uint8_t  data[RING_BUFFER_SIZE];
  uint16_t tail;
  uint16_t head;
}RingBuffer_t;

static void UART_C2C_MspInit(UART_HandleTypeDef *hUART_c2c);

UART_HandleTypeDef huart4;
RingBuffer_t UART_RxData;

// timers
TIM_HandleTypeDef htim2;
static void MX_TIM2_Init(void);
#endif


/* Private variables ---------------------------------------------------------*/
static UART_HandleTypeDef xConsoleUart;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config( void );
static void Console_UART_Init( void );
static void RTC_Init( void );
static void prvWifiConnect( void );

/**
 * @brief Initializes the STM32L475 IoT node board.
 *
 * Initialization of clock, LEDs, RNG, RTC, and WIFI module.
 */
static void prvMiscInitialization( void );

/**
 * @brief Initializes the FreeRTOS heap.
 *
 * Heap_5 is being used because the RAM is not contiguous, therefore the heap
 * needs to be initialized.  See http://www.freertos.org/a00111.html
 */
static void prvInitializeHeap( void );

/**
 * @brief Application runtime entry point.
 */
int main( void )
{
    /* Perform any hardware initialization that does not require the RTOS to be
     * running.  */
    prvMiscInitialization();

    /* Create tasks that are not dependent on the WiFi being initialized. */
    xLoggingTaskInitialize( mainLOGGING_TASK_STACK_SIZE,
                            mainLOGGING_TASK_PRIORITY,
                            mainLOGGING_MESSAGE_QUEUE_LENGTH );

    /* Start the scheduler.  Initialization that requires the OS to be running,
     * including the WiFi initialization, is performed in the RTOS daemon task
     * startup hook. */
    vTaskStartScheduler();

    return 0;
}
/*-----------------------------------------------------------*/


void vApplicationDaemonTaskStartupHook( void )
{
#ifdef USE_ESEYE
		configPRINTF(("Eseye Anynet Weather demo\r\n"));
	/* check modem status */
	C2C_HwStatusInit();

	while(cellular_init() == -1)
	{
		configPRINTF(("Unable to register on cellular, retrying in 30 seconds\r\n"));
		vTaskDelay(30000);
	}
#endif
    /* A simple example to demonstrate key and certificate provisioning in
     * microcontroller flash using PKCS#11 interface. This should be replaced
     * by production ready key provisioning mechanism. */
    vDevModeKeyProvisioning();

    if( SYSTEM_Init() == pdPASS )
    {
        /* Connect to the WiFi before running the demos */
    	if(use_cellular_socket == 0)
    	{
    		prvWifiConnect();
    	}

        DEMO_RUNNER_RunDemos();
    }
}

/*-----------------------------------------------------------*/
static void prvWifiConnect( void )
{
    WIFINetworkParams_t xNetworkParams;
    WIFIReturnCode_t xWifiStatus;
    uint8_t ucIPAddr[ 4 ];

    /* Setup WiFi parameters to connect to access point. */
    xNetworkParams.pcSSID = clientcredentialWIFI_SSID;
    xNetworkParams.ucSSIDLength = sizeof( clientcredentialWIFI_SSID );
    xNetworkParams.pcPassword = clientcredentialWIFI_PASSWORD;
    xNetworkParams.ucPasswordLength = sizeof( clientcredentialWIFI_PASSWORD );
    xNetworkParams.xSecurity = clientcredentialWIFI_SECURITY;
    xNetworkParams.cChannel = 0;

    xWifiStatus = WIFI_On();

    if( xWifiStatus == eWiFiSuccess )
    {
        configPRINTF( ( "WiFi module initialized.\r\n" ) );

        /* Try connecting using provided wifi credentials. */
        xWifiStatus = WIFI_ConnectAP( &( xNetworkParams ) );

        if( xWifiStatus == eWiFiSuccess )
        {
            configPRINTF( ( "WiFi connected to AP %s.\r\n", xNetworkParams.pcSSID ) );

            /* Get IP address of the device. */
            WIFI_GetIP( &ucIPAddr[ 0 ] );

            configPRINTF( ( "IP Address acquired %d.%d.%d.%d\r\n",
                            ucIPAddr[ 0 ], ucIPAddr[ 1 ], ucIPAddr[ 2 ], ucIPAddr[ 3 ] ) );
        }
        else
        {
#if 0
            /* Connection failed configure softAP to allow user to set wifi credentials. */
            configPRINTF( ( "WiFi failed to connect to AP %s.\r\n", xNetworkParams.pcSSID ) );

            xNetworkParams.pcSSID = wificonfigACCESS_POINT_SSID_PREFIX;
            xNetworkParams.pcPassword = wificonfigACCESS_POINT_PASSKEY;
            xNetworkParams.xSecurity = wificonfigACCESS_POINT_SECURITY;
            xNetworkParams.cChannel = wificonfigACCESS_POINT_CHANNEL;

            configPRINTF( ( "Connect to softAP %s using password %s. \r\n",
                            xNetworkParams.pcSSID, xNetworkParams.pcPassword ) );

            while( WIFI_ConfigureAP( &xNetworkParams ) != eWiFiSuccess )
            {
                configPRINTF( ( "Connect to softAP %s using password %s and configure WiFi. \r\n",
                                xNetworkParams.pcSSID, xNetworkParams.pcPassword ) );
            }

            configPRINTF( ( "WiFi configuration successful. \r\n", xNetworkParams.pcSSID ) );
#endif
        }
    }
    else
    {
        configPRINTF( ( "WiFi module failed to initialize.\r\n" ) );
    }
}


/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
 * used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle
     * task's state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetTimerTaskMemory() to provide the memory that is
 * used by the RTOS daemon/time task. */
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle
     * task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
/*-----------------------------------------------------------*/

/**
 * @brief Publishes a character to the STM32L475 UART
 *
 * This is used to implement the tinyprintf created by Spare Time Labs
 * http://www.sparetimelabs.com/tinyprintf/tinyprintf.php
 *
 * @param pv    unused void pointer for compliance with tinyprintf
 * @param ch    character to be printed
 */
void vSTM32L475putc( void * pv,
                     char ch )
{
    while( HAL_OK != HAL_UART_Transmit( &xConsoleUart, ( uint8_t * ) &ch, 1, 30000 ) )
    {
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Initializes the board.
 */
static void prvMiscInitialization( void )
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	int status;
	conn_led = 0;
	got_certs = 0;

    HAL_Init();

    /* Configure the system clock. */
    SystemClock_Config();

    MX_TIM2_Init();

    HAL_NVIC_SetPriority(TIM2_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    HAL_TIM_Base_Init(&htim2);
    HAL_TIM_Base_Start_IT(&htim2);

    /* Heap_5 is being used because the RAM is not contiguous in memory, so the
     * heap must be initialized. */
    prvInitializeHeap();

    BSP_LED_Init( LED1);
    BSP_LED_Init( LED2 );
    BSP_PB_Init( BUTTON_USER, BUTTON_MODE_EXTI );

    /* RNG init function. */
    xHrng.Instance = RNG;

    if( HAL_RNG_Init( &xHrng ) != HAL_OK )
    {
        Error_Handler();
    }

    /* RTC init. */
    RTC_Init();

    /* UART console init. */
    Console_UART_Init();
}
/*-----------------------------------------------------------*/

/**
 * @brief Initializes the system clock.
 */
#ifdef USE_ESEYE
static void SystemClock_Config( void )
{
    RCC_OscInitTypeDef xRCC_OscInitStruct;
    RCC_ClkInitTypeDef xRCC_ClkInitStruct;
    RCC_PeriphCLKInitTypeDef xPeriphClkInit;

    xRCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
    xRCC_OscInitStruct.LSEState = RCC_LSE_ON;
    xRCC_OscInitStruct.LSIState = RCC_LSI_ON;
    xRCC_OscInitStruct.MSIState = RCC_MSI_ON;
    xRCC_OscInitStruct.MSICalibrationValue = 0;
    xRCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
    xRCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    xRCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    xRCC_OscInitStruct.PLL.PLLM = 6;
    xRCC_OscInitStruct.PLL.PLLN = 20;
    xRCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    xRCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    xRCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

    if( HAL_RCC_OscConfig( &xRCC_OscInitStruct ) != HAL_OK )
    {
        Error_Handler();
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     * clocks dividers. */
    xRCC_ClkInitStruct.ClockType = ( RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 );
    xRCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    xRCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    xRCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    xRCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if( HAL_RCC_ClockConfig( &xRCC_ClkInitStruct, FLASH_LATENCY_4 ) != HAL_OK )
    {
        Error_Handler();
    }

    xPeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC
                                          | RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART3
										  | RCC_PERIPHCLK_UART4 | RCC_PERIPHCLK_I2C2
                                          | RCC_PERIPHCLK_RNG;
    xPeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
    xPeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
    xPeriphClkInit.Uart4ClockSelection = RCC_UART4CLKSOURCE_PCLK1;
    xPeriphClkInit.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
    xPeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    xPeriphClkInit.RngClockSelection = RCC_RNGCLKSOURCE_MSI;

    if( HAL_RCCEx_PeriphCLKConfig( &xPeriphClkInit ) != HAL_OK )
    {
        Error_Handler();
    }

    __HAL_RCC_PWR_CLK_ENABLE();

    if( HAL_PWREx_ControlVoltageScaling( PWR_REGULATOR_VOLTAGE_SCALE1 ) != HAL_OK )
    {
        Error_Handler();
    }

    /* Enable MSI PLL mode. */
    HAL_RCCEx_EnableMSIPLLMode();
}
#else
static void SystemClock_Config( void )
{
    RCC_OscInitTypeDef xRCC_OscInitStruct;
    RCC_ClkInitTypeDef xRCC_ClkInitStruct;
    RCC_PeriphCLKInitTypeDef xPeriphClkInit;

    xRCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
    xRCC_OscInitStruct.LSEState = RCC_LSE_ON;
    xRCC_OscInitStruct.MSIState = RCC_MSI_ON;
    xRCC_OscInitStruct.MSICalibrationValue = 0;
    xRCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
    xRCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    xRCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    xRCC_OscInitStruct.PLL.PLLM = 6;
    xRCC_OscInitStruct.PLL.PLLN = 20;
    xRCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    xRCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    xRCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

    if( HAL_RCC_OscConfig( &xRCC_OscInitStruct ) != HAL_OK )
    {
        Error_Handler();
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     * clocks dividers. */
    xRCC_ClkInitStruct.ClockType = ( RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 );
    xRCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    xRCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    xRCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    xRCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if( HAL_RCC_ClockConfig( &xRCC_ClkInitStruct, FLASH_LATENCY_4 ) != HAL_OK )
    {
        Error_Handler();
    }

    xPeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC
                                          | RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART3 | RCC_PERIPHCLK_I2C2
                                          | RCC_PERIPHCLK_RNG;
    xPeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
    xPeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
    xPeriphClkInit.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
    xPeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    xPeriphClkInit.RngClockSelection = RCC_RNGCLKSOURCE_MSI;

    if( HAL_RCCEx_PeriphCLKConfig( &xPeriphClkInit ) != HAL_OK )
    {
        Error_Handler();
    }

    __HAL_RCC_PWR_CLK_ENABLE();

    if( HAL_PWREx_ControlVoltageScaling( PWR_REGULATOR_VOLTAGE_SCALE1 ) != HAL_OK )
    {
        Error_Handler();
    }

    /* Enable MSI PLL mode. */
    HAL_RCCEx_EnableMSIPLLMode();
}
#endif
/*-----------------------------------------------------------*/

/**
 * @brief UART console initialization function.
 */
static void Console_UART_Init( void )
{
    xConsoleUart.Instance = USART1;
    xConsoleUart.Init.BaudRate = 115200;
    xConsoleUart.Init.WordLength = UART_WORDLENGTH_8B;
    xConsoleUart.Init.StopBits = UART_STOPBITS_1;
    xConsoleUart.Init.Parity = UART_PARITY_NONE;
    xConsoleUart.Init.Mode = UART_MODE_TX_RX;
    xConsoleUart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    xConsoleUart.Init.OverSampling = UART_OVERSAMPLING_16;
    xConsoleUart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    xConsoleUart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    BSP_COM_Init( COM1, &xConsoleUart );
}
/*-----------------------------------------------------------*/

/**
 * @brief RTC init function.
 */
static void RTC_Init( void )
{
    RTC_TimeTypeDef xsTime;
    RTC_DateTypeDef xsDate;

    /* Initialize RTC Only. */
    xHrtc.Instance = RTC;
    xHrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    xHrtc.Init.AsynchPrediv = 127;
    xHrtc.Init.SynchPrediv = 255;
    xHrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    xHrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    xHrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    xHrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if( HAL_RTC_Init( &xHrtc ) != HAL_OK )
    {
        Error_Handler();
    }

    /* Initialize RTC and set the Time and Date. */
    xsTime.Hours = 0x12;
    xsTime.Minutes = 0x0;
    xsTime.Seconds = 0x0;
    xsTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    xsTime.StoreOperation = RTC_STOREOPERATION_RESET;

    if( HAL_RTC_SetTime( &xHrtc, &xsTime, RTC_FORMAT_BCD ) != HAL_OK )
    {
        Error_Handler();
    }

    xsDate.WeekDay = RTC_WEEKDAY_FRIDAY;
    xsDate.Month = RTC_MONTH_JANUARY;
    xsDate.Date = 0x24;
    xsDate.Year = 0x17;

    if( HAL_RTC_SetDate( &xHrtc, &xsDate, RTC_FORMAT_BCD ) != HAL_OK )
    {
        Error_Handler();
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief  This function is executed in case of error occurrence.
 */
void Error_Handler( void )
{
    while( 1 )
    {
        BSP_LED_Toggle( LED_GREEN );
        HAL_Delay( 200 );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Warn user if pvPortMalloc fails.
 *
 * Called if a call to pvPortMalloc() fails because there is insufficient
 * free memory available in the FreeRTOS heap.  pvPortMalloc() is called
 * internally by FreeRTOS API functions that create tasks, queues, software
 * timers, and semaphores.  The size of the FreeRTOS heap is set by the
 * configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h.
 *
 */
void vApplicationMallocFailedHook()
{
    taskDISABLE_INTERRUPTS();
    for( ;; );
}
/*-----------------------------------------------------------*/

/**
 * @brief Loop forever if stack overflow is detected.
 *
 * If configCHECK_FOR_STACK_OVERFLOW is set to 1,
 * this hook provides a location for applications to
 * define a response to a stack overflow.
 *
 * Use this hook to help identify that a stack overflow
 * has occurred.
 *
 */
void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    char * pcTaskName )
{
    portDISABLE_INTERRUPTS();

    /* Loop forever */
    for( ; ; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
}
/*-----------------------------------------------------------*/

void * malloc( size_t xSize )
{
    configASSERT( xSize == ~0 );

    return NULL;
}
/*-----------------------------------------------------------*/


void vOutputChar( const char cChar,
                  const TickType_t xTicksToWait )
{
    ( void ) cChar;
    ( void ) xTicksToWait;
}
/*-----------------------------------------------------------*/

void vMainUARTPrintString( char * pcString )
{
    const uint32_t ulTimeout = 3000UL;

    HAL_UART_Transmit( &xConsoleUart,
                       ( uint8_t * ) pcString,
                       strlen( pcString ),
                       ulTimeout );
}
/*-----------------------------------------------------------*/

void prvGetRegistersFromStack( uint32_t * pulFaultStackAddress )
{
/* These are volatile to try and prevent the compiler/linker optimising them
 * away as the variables never actually get used.  If the debugger won't show the
 * values of the variables, make them global my moving their declaration outside
 * of this function. */
    volatile uint32_t r0;
    volatile uint32_t r1;
    volatile uint32_t r2;
    volatile uint32_t r3;
    volatile uint32_t r12;
    volatile uint32_t lr;  /* Link register. */
    volatile uint32_t pc;  /* Program counter. */
    volatile uint32_t psr; /* Program status register. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    /* Remove compiler warnings about the variables not being used. */
    ( void ) r0;
    ( void ) r1;
    ( void ) r2;
    ( void ) r3;
    ( void ) r12;
    ( void ) lr;  /* Link register. */
    ( void ) pc;  /* Program counter. */
    ( void ) psr; /* Program status register. */

    /* When the following line is hit, the variables contain the register values. */
    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

/* The fault handler implementation calls a function called
 * prvGetRegistersFromStack(). */
void HardFault_Handler( void )
{
    __asm volatile
    (
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
}
/*-----------------------------------------------------------*/

/* Psuedo random number generator.  Just used by demos so does not need to be
 * secure.  Do not use the standard C library rand() function as it can cause
 * unexpected behaviour, such as calls to malloc(). */
int iMainRand32( void )
{
    static UBaseType_t uxlNextRand; /*_RB_ Not seeded. */
    const uint32_t ulMultiplier = 0x015a4e35UL, ulIncrement = 1UL;

    /* Utility function to generate a pseudo random number. */

    uxlNextRand = ( ulMultiplier * uxlNextRand ) + ulIncrement;

    return( ( int ) ( uxlNextRand >> 16UL ) & 0x7fffUL );
}
/*-----------------------------------------------------------*/

static void prvInitializeHeap( void )
{
    static uint8_t ucHeap1[ configTOTAL_HEAP_SIZE ];
    static uint8_t ucHeap2[ 26 * 1024 ] __attribute__( ( section( ".freertos_heap2" ) ) );

    HeapRegion_t xHeapRegions[] =
    {
        { ( unsigned char * ) ucHeap2, sizeof( ucHeap2 ) },
        { ( unsigned char * ) ucHeap1, sizeof( ucHeap1 ) },
        { NULL,                                        0 }
    };

    vPortDefineHeapRegions( xHeapRegions );
}
/*-----------------------------------------------------------*/

#ifdef USE_ESEYE

/* TIM2 init function */
static void MX_TIM2_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  __TIM2_CLK_ENABLE();
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 40000;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
	  Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
	  Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
	  Error_Handler();
  }

}


static void UART_C2C_MspDeInit(UART_HandleTypeDef *hUART_c2c)
{
  //static DMA_HandleTypeDef hdma_tx;

  /*##-1- Reset peripherals ##################################################*/
  UART_C2C_FORCE_RESET();
  UART_C2C_RELEASE_RESET();

  /*##-2- Disable peripherals and GPIO Clocks #################################*/
  /* Configure UART Tx as alternate function  */
  HAL_GPIO_DeInit(UART_C2C_TX_GPIO_PORT, UART_C2C_TX_PIN);
  /* Configure UART Rx as alternate function  */
  HAL_GPIO_DeInit(UART_C2C_RX_GPIO_PORT, UART_C2C_RX_PIN);

  /*##-3- Disable the DMA Channels ###########################################*/
  /* De-Initialize the DMA Channel associated to transmission process */
  //HAL_DMA_DeInit(&hdma_tx);

  /*##-4- Disable the NVIC for DMA ###########################################*/
  HAL_NVIC_DisableIRQ(UART_C2C_DMA_TX_IRQn);
}

int8_t UART_C2C_Init(void)
{
  /* Set the C2C USART configuration parameters on MCU side */
  /* Attention: make sure the module uart is configured with the same values */
	huart4.Instance        = UART4;
	huart4.Init.BaudRate   = UG96_DEFAULT_BAUDRATE;
	huart4.Init.WordLength = UART_WORDLENGTH_8B;
	huart4.Init.StopBits   = UART_STOPBITS_1;
	huart4.Init.Parity     = UART_PARITY_NONE;
	huart4.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
	huart4.Init.Mode       = UART_MODE_TX_RX;
	huart4.Init.OverSampling = UART_OVERSAMPLING_16;
	huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  UART_C2C_MspInit(&huart4);
  /* Configure the USART IP */
  if(HAL_UART_Init(&huart4) != HAL_OK)
  {
    return -1;
  }

  /* Once the C2C UART is initialized, start an asynchronous recursive
   listening. the HAL_UART_Receive_IT() call below will wait until one char is
   received to trigger the HAL_UART_RxCpltCallback(). The latter will recursively
   call the former to read another char.  */
  UART_RxData.head = 0;
  UART_RxData.tail = 0;
  HAL_UART_Receive_IT(&huart4, (uint8_t *)&UART_RxData.data[UART_RxData.tail], 1);

  return 0;
}

static void UART_C2C_MspInit(UART_HandleTypeDef *hUART_c2c)
{
  //static DMA_HandleTypeDef hdma_tx;
  GPIO_InitTypeDef  GPIO_Init;

  /* Enable the GPIO clock */
  /* C2C_RST_GPIO_CLK_ENABLE(); */


  /* Set the GPIO pin configuration parametres */
//  GPIO_Init.Pin       = C2C_RST_PIN;
//  GPIO_Init.Mode      = GPIO_MODE_OUTPUT_PP;
//  GPIO_Init.Pull      = GPIO_PULLUP;
//  GPIO_Init.Speed     = GPIO_SPEED_HIGH;
//
//  /* Configure the RST IO */
//  HAL_GPIO_Init(C2C_RST_GPIO_PORT, &GPIO_Init);

  /* Enable DMA clock */
  //DMAx_CLK_ENABLE();

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO TX/RX clock */
  //HAL_PWREx_EnableVddIO2(); /* needed for GPIO PGxx on L496AG*/
  UART_C2C_TX_GPIO_CLK_ENABLE();
  UART_C2C_RX_GPIO_CLK_ENABLE();

  /* Enable UART_C2C clock */
  UART_C2C_CLK_ENABLE();

  /* Enable DMA clock */
  //DMAx_CLK_ENABLE();

  /*##-2- Configure peripheral GPIO ##########################################*/
  /* UART TX GPIO pin configuration  */
  GPIO_Init.Pin       = UART_C2C_TX_PIN|UART_C2C_RX_PIN;
  GPIO_Init.Mode      = GPIO_MODE_AF_PP;
  GPIO_Init.Pull      = GPIO_NOPULL;
  GPIO_Init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_Init.Alternate = UART_C2C_TX_AF;

  HAL_GPIO_Init(UART_C2C_TX_GPIO_PORT, &GPIO_Init);

  /* UART RX GPIO pin configuration  */
 // GPIO_Init.Pin = UART_C2C_RX_PIN;
  //GPIO_Init.Alternate = UART_C2C_RX_AF;

  //HAL_GPIO_Init(UART_C2C_RX_GPIO_PORT, &GPIO_Init);


  /*##-3- Configure the NVIC for UART ########################################*/
  HAL_NVIC_SetPriority(UART_C2C_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(UART_C2C_IRQn);

}

int8_t UART_C2C_DeInit(void)
{
  /* Reset USART configuration to default */
  HAL_UART_DeInit(&huart4);
  UART_C2C_MspDeInit(&huart4);

  return 0;
}

/**
  * @brief  C2C IO change baudrate
  *         To be used just in case the UG96_DEFAULT_BAUDRATE need to be changed
  *         This function has to be called after having changed the C2C module baudrate
  *         In order to do that the SMT32 Init shall be done at UG96_DEFAULT_BAUDRATE
  *         After C2C module baudrate is changed this function sets the STM32 baudrate accordingly
  * @param  None.
  * @retval 0 on success, -1 otherwise.
  */
int8_t UART_C2C_SetBaudrate(uint32_t BaudRate)
{
  HAL_UART_DeInit(&huart4);
  huart4.Init.BaudRate   = BaudRate;
  if(HAL_UART_Init(&huart4) != HAL_OK)
  {
    return -1;
  }
  /* Once the C2C UART is initialized, start an asynchronous recursive
   listening. the HAL_UART_Receive_IT() call below will wait until one char is
   received to trigger the HAL_UART_RxCpltCallback(). The latter will recursively
   call the former to read another char.  */
  UART_RxData.head = 0;
  UART_RxData.tail = 0;
  HAL_UART_Receive_IT(&huart4, (uint8_t *)&UART_RxData.data[UART_RxData.tail], 1);

  return 0;
}


/**
  * @brief  Flush Ring Buffer
  * @param  None
  * @retval None.
  */
void UART_C2C_FlushBuffer(void)
{
  memset(UART_RxData.data, 0, RING_BUFFER_SIZE);
  UART_RxData.head = UART_RxData.tail = 0;
}

/**
  * @brief  Send Data to the C2C module over the UART interface.
  *         This function allows sending data to the  C2C Module, the
  *         data can be either an AT command or raw data to send over
  *         a pre-established C2C connection.
  * @param pData: data to send.
  * @param Length: the data length.
  * @retval 0 on success, -1 otherwise.
  */
int16_t UART_C2C_SendData(uint8_t* pData, uint16_t Length)
{
  if (HAL_UART_Transmit(&huart4, (uint8_t*)pData, Length, 2000) != HAL_OK)
  {
     return -1;
  }

  return 0;
}


/**
  * @brief  Retrieve on Data from intermediate IT buffer
  * @param pData: data to send.
  * @retval 0 data available, -1 no data to retrieve
  */
int16_t  UART_C2C_ReceiveSingleData(uint8_t* pSingleData)
{
  /* Note: other possible implementation is to retrieve directly one data from UART buffer */
  /* without using the interrupt and the intermediate buffer */

  if(UART_RxData.head != UART_RxData.tail)
  {
    /* serial data available, so return data to user */
    *pSingleData = UART_RxData.data[UART_RxData.head++];

    /* check for ring buffer wrap */
    if (UART_RxData.head >= RING_BUFFER_SIZE)
    {
      /* ring buffer wrap, so reset head pointer to start of buffer */
      UART_RxData.head = 0;
    }
  }
  else
  {
   return -1;
  }

  return 0;
}


/**
  * @brief  Rx Callback when new data is received on the UART.
  * @param  UartHandle: Uart handle receiving the data.
  * @retval None.
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartH)
{
  /* If ring buffer end is reached reset tail pointer to start of buffer */
  if(++UART_RxData.tail >= RING_BUFFER_SIZE)
  {
    UART_RxData.tail = 0;
  }
  if(UART_RxData.tail == UART_RxData.head)
  {
	  ++UART_RxData.head;
	  if (UART_RxData.head >= RING_BUFFER_SIZE)
	  {
		  /* ring buffer wrap, so reset head pointer to start of buffer */
		  UART_RxData.head = 0;
	  }
  }
  HAL_UART_Receive_IT(UartH, (uint8_t *)&UART_RxData.data[UART_RxData.tail], 1);
}

#endif
