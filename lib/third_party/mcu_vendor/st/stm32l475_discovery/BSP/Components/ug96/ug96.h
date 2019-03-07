/**
  ******************************************************************************
  * @file    ug96.h
  * @author  MCD Application Team
  * @version V0.0.1
  * @date    11-08-2017
  * @brief   header file for the ug96 module (C2C cellular modem).
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
#ifndef __UG96_H
#define __UG96_H

#ifdef __cplusplus
 extern "C" {
#endif  

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "string.h"
#include "stdio.h"
#include "ug96_conf.h"


/* Private Constants --------------------------------------------------------*/
#define  RET_NONE           0x0000  /* RET_NONE shall be 0x0: don't change this value! */
#define  RET_CRLF           0x0001
#define  RET_OK             0x0002  /* do not change this value */
#define  RET_SIM_READY      0x0004
#define  RET_ARROW          0x0008
#define  RET_SENT           0x0010  /* do not change this value */
#define  RET_OPEN           0x0020
#define  RET_ERROR          0x0040  /* do not change this value */
#define  RET_URC_CLOSED     0x0080
#define  RET_URC_RECV       0x0100
#define  RET_URC_IN_FULL    0x0200
#define  RET_URC_INCOM      0x0400
#define  RET_URC_PDPDEACT   0x0800
#define  RET_URC_DNS        0x1000
#define  RET_BUF_FULL       0x2000  /* do not change this value */
#define  RET_READ           0x4000
#define  RET_CME_ERROR      0x8000  /* do not change this value */
#define  RET_CMS_ERROR      0x10000
#define  RET_POWERED_DOWN   0x20000
#define  RET_SEND           0x40000
#define  NUM_RESPONSES  18


/* Timeouts modem dependent */
#define UG96_TOUT_SHORT                         50  /* 50 ms */
#define UG96_TOUT_300                          350  /* 0,3 sec + margin */
#define UG96_TOUT_ATSYNC                       500
/* Timeouts network dependent */   
#define UG96_TOUT_5000                        5500  /* 5 sec + margin */
#define UG96_TOUT_15000                      16500  /* 15 sec + margin */
#define UG96_TOUT_40000                      42000  /* 40 sec + margin */
#define UG96_TOUT_60000                      64000  /* 1 min + margin */
#define UG96_TOUT_75000                      78000  /* 75 sec + margin */
#define UG96_TOUT_150000                    156000  /* 2,5 min + margin */
#define UG96_TOUT_180000                    186000  /* 3 min + margin */


/* Exported constants---------------------------------------------------------*/

#define UG96_MAX_APN_NAME_SIZE                  32
#define UG96_MAX_USER_NAME_SIZE                 32
#define UG96_MAX_PSW_NAME_SIZE                  32

#define UG96_ERROR_STRING_SIZE                  40

#define UG96_MFC_SIZE                           10
#define UG96_PROD_ID_SIZE                        6
#define UG96_FW_REV_SIZE                        16
#define UG96_IMEI_SIZE                          16
#define UG96_ICCID_SIZE                         20
#define UG96_IMSI_SIZE                          16

/* Exported macro-------------------------------------------------------------*/
#define MIN(a, b)  ((a) < (b) ? (a) : (b))


/* Exported typedef ----------------------------------------------------------*/   
typedef int8_t (*IO_Init_Func)( void);
typedef int8_t (*IO_DeInit_Func)( void);
typedef int8_t (*IO_Baudrate_Func)(uint32_t BaudRate);
typedef void (*IO_Flush_Func)(void);
typedef int16_t (*IO_Send_Func)( uint8_t *, uint16_t);
typedef int16_t (*IO_ReceiveOne_Func)(uint8_t* pSingleData);
typedef uint32_t (*App_GetTickCb_Func)(void);


typedef struct {
  uint32_t retval;
  char retstr[100];
} UG96_RetKeywords_t;

typedef enum {
  UG96_RETURN_OK            = RET_OK,         /*shall be aligned with above definitions */
  UG96_RETURN_ERROR         = RET_ERROR,      /*shall be aligned with above definitions */
  UG96_RETURN_CME_ERROR     = RET_CME_ERROR,  /*shall be aligned with above definitions */
  UG96_RETURN_RETRIEVE_ERROR = -1,
  UG96_RETURN_SEND_ERROR    = -2
}UG96_Return_t;

typedef enum {   /* See CME Error codes */
  UG96_SIM_ERROR           = 0,
  UG96_SIM_NOT_INSERTED    = 10,
  UG96_SIM_PIN_REQUIRED    = 11,
  UG96_SIM_PUK_REQUIRED    = 12,
  UG96_SIM_FAILURE         = 13,
  UG96_SIM_BUSY            = 14,
  UG96_SIM_WRONG           = 15,
  UG96_INCORRECT_PSW       = 16,
  UG96_SIM_PIN2_REQUIRED   = 17,
  UG96_SIM_PUK2_REQUIRED   = 18,
  UG96_OPERATION_NOT_ALLOW = 3,
  UG96_SIM_READY           = 0xFF
} UG96_SIMState_t;

typedef enum {
  UG96_NRS_NOT_REGISTERED  = 0x00,
  UG96_NRS_HOME_NETWORK    = 0x01,
  UG96_NRS_TRYING          = 0x02,
  UG96_NRS_REG_DENIED      = 0x03,
  UG96_NRS_UNKNOWN         = 0x04,
  UG96_NRS_ROAMING         = 0x05,
  UG96_NRS_ERROR           = 0xFF
} UG96_NetworkRegistrationState_t;

typedef enum {
  UG96_AP_NOT_CONFIG  = 0x00,
  UG96_AP_CONFIGURED  = 0x01,
  UG96_AP_ACVTIVATED  = 0x02,
  UG96_AP_ERROR       = 0xFF
} UG96_APState_t;

typedef enum {
  UG96_INIT_RET_OK        = RET_OK,       /*shall be aligned with above definitions */
  UG96_INIT_RET_AT_ERR    = 0x04,
  UG96_INIT_RET_SIM_ERR   = 0x08,        
  UG96_INIT_RET_IO_ERR    = 0x10,
  UG96_INIT_OTHER_ERR     = 0x20
}UG96_InitRet_t;

typedef enum {
  UG96_SEND_RET_UART_FAIL  = 0x1,
  UG96_SEND_RET_SENT       = RET_SENT,     /*shall be aligned with above definitions */
  UG96_SEND_RET_BUF_FULL   = RET_BUF_FULL, /*shall be aligned with above definitions */
  UG96_SEND_RET_CONN_ERR   = RET_ERROR     /*shall be aligned with above definitions */
}UG96_SendRet_t;

typedef enum {
  UG96_RECEIVE_RET_INCOMPLETE    = 0x01,
  UG96_RECEIVE_RET_OK            = RET_OK, /*shall be aligned with above definitions */
  UG96_RECEIVE_RET_PARAM_ERR     = 0x04,
  UG96_RECEIVE_RET_COM_ERR       = 0x08
}UG96_ReceiveRet_t;

typedef enum {
  UG96_TCP_CONNECTION           = 0,
  UG96_UDP_CONNECTION           = 1,
  UG96_TCP_LISTENER_CONNECTION  = 2,
  UG96_UDP_SERVER_CONNECTION    = 3
}UG96_ConnType_t;

typedef enum {
  UG96_BUFFER_MODE           = 0,
  UG96_DIRECT_PUSH           = 1,
  UG96_TRANSPARENT_MODE      = 2
}UG96_AccessMode_t;

/**
 * \brief  Authentication settings for C2C network
 */
typedef enum {
  UG96_AUTHENT_NONE     = 0x00,
  UG96_AUTHENT_PAP      = 0x01,
  UG96_AUTHENT_CHAP     = 0x02,
  UG96_AUTHENT_PAP_CHAP = 0x03
}UG96_Authent_t;

typedef enum {
  UG96_UART_FLW_CTL_NONE     = 0x00,   
  UG96_UART_FLW_CTL_RTS      = 0x01, 
  UG96_UART_FLW_CTL_CTS      = 0x02,  
  UG96_UART_FLW_CTL_RTS_CTS  = 0x03,    
} UG96_UART_FLW_CTL_t;

typedef struct
{
  uint32_t           BaudRate;          
  uint32_t           FlowControl;   
}UG96_UARTConfig_t;

typedef struct {
  uint8_t            ContextID;      /*!< range is 1-20 */
  uint8_t            ContextType;    /*!< shall be 1 (IpV */
  uint8_t            ApnString[UG96_MAX_APN_NAME_SIZE];  /*!< access point name, string of chars */
  uint8_t            Username[UG96_MAX_USER_NAME_SIZE];   /*!< user name, string of chars */
  uint8_t            Password[UG96_MAX_PSW_NAME_SIZE];   /*!< password, string of chars */
  UG96_Authent_t     Authentication;
} UG96_APConfig_t;

typedef struct {
  UG96_ConnType_t    Type;     
  UG96_AccessMode_t  AccessMode;  
  uint8_t            ConnectID;             
  uint16_t           RemotePort;         
  uint16_t           LocalPort;          
  char*              Url;  
} UG96_Conn_t;

typedef struct {
  UG96_ConnType_t    Type;     
  UG96_AccessMode_t  AccessMode;
  uint16_t           ComulatedQirdData;
  uint16_t           HaveReadLength;
  uint16_t           UnreadLength;
  int16_t            UartRemaining; /* if Timeout respects UART speed this should always be 0 */  
} UG96_Socket_t;

typedef struct {
  IO_Init_Func       IO_Init;  
  IO_DeInit_Func     IO_DeInit;
  IO_Baudrate_Func   IO_Baudrate;
  IO_Flush_Func      IO_FlushBuffer;  
  IO_Send_Func       IO_Send;
  IO_ReceiveOne_Func IO_ReceiveOne;  
} UG96_IO_t;

typedef struct {
  UG96_SIMState_t    SimStatus;  
  //MC uint8_t            RegistStatusString[3];
  uint8_t            IMSI [UG96_IMSI_SIZE+1];
  uint8_t            ICCID [UG96_ICCID_SIZE+1];
} UG96_SIMInfo_t;

typedef struct {
  uint8_t             Manufacturer[UG96_MFC_SIZE];
  uint8_t             ProductID[UG96_PROD_ID_SIZE];
  uint8_t             FW_Rev[UG96_FW_REV_SIZE];
  uint8_t             Imei[UG96_IMEI_SIZE];               /*International Mobile Equipment Identity*/
  UG96_SIMInfo_t      SimInfo;   
  uint8_t             APsActive;
  uint8_t             APContextState[UG96_MAX_CONTEXTS];  /* to decide if keeping all UG96_APConfig_t info. maybe at c2c SW level*/  
  UG96_Socket_t       SocketInfo[UG96_MAX_SOCKETS];       /* to decide if keeping all UG96_Conn_t info. maybe at c2c SW level*/
  UG96_UARTConfig_t   UART_Config;  
  UG96_IO_t           fops;
  App_GetTickCb_Func  GetTickCb;
  uint8_t             CmdResp[UG96_CMD_SIZE];
  uint32_t            RemainRxData;
}Ug96Object_t;

/* Exported functions --------------------------------------------------------*/

/* ==== Init and status ==== */

UG96_Return_t  UG96_RegisterBusIO(Ug96Object_t *Obj, IO_Init_Func IO_Init,
                                                     IO_DeInit_Func IO_DeInit,
                                                     IO_Baudrate_Func IO_Baudrate,
                                                     IO_Send_Func IO_Send,
                                                     IO_ReceiveOne_Func IO_ReceiveOne,
                                                     IO_Flush_Func IO_Flush);

UG96_InitRet_t UG96_Init(Ug96Object_t *Obj);

UG96_Return_t UG96_PowerDown(Ug96Object_t *Obj);

/* ==== Registration and network selection ==== */

UG96_Return_t  UG96_GetSignalQualityStatus(Ug96Object_t *Obj, int32_t *Qvalue);
UG96_Return_t  UG96_PSAttach(Ug96Object_t *Obj);
UG96_Return_t  UG96_AutomaticPlmnSelection(Ug96Object_t *Obj);
UG96_Return_t  UG96_NetworkSearch(Ug96Object_t *Obj);
UG96_Return_t  UG96_NetworkDisplay(Ug96Object_t *Obj);
UG96_Return_t  UG96_SetFullFunctionality(Ug96Object_t *Obj);
UG96_NetworkRegistrationState_t  UG96_GetCsNetworkRegistrationStatus(Ug96Object_t *Obj);
UG96_NetworkRegistrationState_t  UG96_GetPsNetworkRegistrationStatus(Ug96Object_t *Obj);


UG96_Return_t  UG96_ListOperators(Ug96Object_t *Obj, char *Operators);
UG96_Return_t  UG96_GetCurrentOperator(Ug96Object_t *Obj, char *Operator, uint8_t Bufsize);
UG96_Return_t  UG96_ForceOperator(Ug96Object_t *Obj, int32_t OperatorCode);

/* ==== AP Connection ==== */

UG96_Return_t   UG96_ConfigureAP(Ug96Object_t *Obj, UG96_APConfig_t *ApConfig);
UG96_Return_t   UG96_Activate(Ug96Object_t *Obj, uint8_t ContextID);
UG96_Return_t   UG96_Deactivate(Ug96Object_t *Obj, uint8_t ContextID);
UG96_APState_t  UG96_IsActivated(Ug96Object_t *Obj, uint8_t ContextID);

/* ====IP Addr ==== */

UG96_Return_t   UG96_GetActiveIpAddresses(Ug96Object_t *Obj, char *IPaddr_string, uint8_t* IPaddr_int);

/* ==== Ping ==== */

#if (UG96_USE_PING == 1)
UG96_Return_t  UG96_Ping(Ug96Object_t *Obj, uint8_t ContextID, char *host_addr_string, uint16_t count, uint16_t rep_delay_sec);
#endif

/* ==== Client connection ==== */

UG96_Return_t      UG96_DNS_LookUp(Ug96Object_t *Obj, uint8_t ContextID, const char *IPaddr_string, uint8_t* IPaddr_int);
UG96_Return_t      UG96_OpenClientConnection(Ug96Object_t *Obj, uint8_t ContextID, UG96_Conn_t *conn);
UG96_Return_t      UG96_CloseClientConnection(Ug96Object_t *Obj, UG96_Conn_t *conn);

UG96_SendRet_t     UG96_SendData(Ug96Object_t *Obj, uint8_t Socket, uint8_t *pdata, uint16_t Reqlen , uint16_t *SentLen , uint32_t Timeout);
UG96_ReceiveRet_t  UG96_ReceiveData(Ug96Object_t *Obj, uint8_t Socket, uint8_t *pdata, uint16_t Reqlen, uint16_t *Receivedlen, uint32_t Timeout);

/* ==== Miscellaneus ==== */

UG96_Return_t  UG96_ResetToFactoryDefault(Ug96Object_t *Obj);
UG96_Return_t  UG96_SetUARTBaudrate(Ug96Object_t *Obj, UG96_UARTConfig_t *pconf);
UG96_Return_t  UG96_GetUARTConfig(Ug96Object_t *Obj, UG96_UARTConfig_t *pconf);

void           UG96_GetManufacturer( Ug96Object_t *Obj, uint8_t *Manufacturer);
void           UG96_GetProductID(Ug96Object_t *Obj, uint8_t *ProductID);
void           UG96_GetFWRevID(Ug96Object_t *Obj, uint8_t *Fw_ver);

UG96_Return_t  UG96_RetrieveLastErrorDetails(Ug96Object_t *Obj, char *error_string);

/* Application must provide callback function that gives a Timer Tick in ms (e.g. HAL_GetTick())*/
UG96_Return_t  UG96_RegisterTickCb(Ug96Object_t *Obj, App_GetTickCb_Func  GetTickCb);


/* eseye */
UG96_Return_t  UG96_GetCrsmData(Ug96Object_t *Obj, int ef, unsigned int offset, unsigned int length);
#ifdef __cplusplus
}
#endif
#endif /*__UG96_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/ 
