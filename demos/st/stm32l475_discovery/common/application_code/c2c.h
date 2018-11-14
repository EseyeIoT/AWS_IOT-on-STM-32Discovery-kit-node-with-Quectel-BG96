/**
  ******************************************************************************
  * @file    c2c.h
  * @author  MCD Application Team
  * @brief   This file contains the different c2c core resource definitions.
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
#ifndef __C2C_H_
#define __C2C_H_

#ifdef __cplusplus
 extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/   
//#include "ug96.h"
//#include "ug96_io.h"
#include <stdint.h>
/* Exported constants --------------------------------------------------------*/

#define C2C_MAX_DELAY      0xFFFFU

#define C2C_OPERATOR_STRING          30
#define C2C_OPERATORS_LIST           6*C2C_OPERATOR_STRING
#define C2C_IPADDR_STRING            35
#define C2C_IPADDR_LIST              3*C2C_IPADDR_STRING
#define C2C_ERROR_STRING             40

#define C2C_MFC_SIZE                 16
#define C2C_PROD_ID_SIZE             16
#define C2C_FW_REV_SIZE              16
#define C2C_IMEI_SIZE                16
#define C2C_ICCID_SIZE               20
   
#define C2C_MAX_NOS                  100  /* network operators */
#define C2C_MAX_CONNECTIONS          UG96_MAX_SOCKETS
#define C2C_PAYLOAD_SIZE             1500 /* should be = or > then UG96_RX_DATABUF_SIZE */

/* Exported types ------------------------------------------------------------*/
typedef enum {
  C2C_IPV4 = 0x01,
  C2C_IPV6 = 0x02
} C2C_IPVer_t;

typedef enum {
  C2C_AUTHENT_NONE     = 0x00,
  C2C_AUTHENT_PAP      = 0x01,
  C2C_AUTHENT_CHAP     = 0x02,
  C2C_AUTHENT_PAP_CHAP = 0x03
}C2C_Authent_t;

typedef enum {
  C2C_TCP_PROTOCOL = 0,
  C2C_UDP_PROTOCOL = 1,
  C2C_TCP_LISTENER_PROTOCOL = 2, 
  C2C_UDP_SERVICE_PROTOCOL = 3   
}C2C_Protocol_t;

typedef enum {        /* Registration Status */
  C2C_REGISTATUS_NOT_REGISTERED      = 0x00,
  C2C_REGISTATUS_HOME_NETWORK        = 0x01,
  C2C_REGISTATUS_TRYING              = 0x02,
  C2C_REGISTATUS_REG_DENIED          = 0x03,
  C2C_REGISTATUS_UNKNOWN             = 0x04,
  C2C_REGISTATUS_ROAMING             = 0x05,
  C2C_REGISTATUS_SIM_NOT_INSERTED    = 0x0A,
  C2C_REGISTATUS_SIM_PIN_REQUIRED    = 0x0B,
  C2C_REGISTATUS_SIM_PUK_REQUIRED    = 0x0C,
  C2C_REGISTATUS_SIM_FAILURE         = 0x0D,
  C2C_REGISTATUS_SIM_BUSY            = 0x0E,
  C2C_REGISTATUS_SIM_WRONG           = 0x0F,
  C2C_REGISTATUS_INCORRECT_PSW       = 0x10,
  C2C_REGISTATUS_SIM_PIN2_REQUIRED   = 0x11,
  C2C_REGISTATUS_SIM_PUK2_REQUIRED   = 0x12,
  C2C_REGISTATUS_SIM_OP_NOT_ALLOWED  = 0x13,
  C2C_REGISTATUS_ERROR               = 0xFF
} C2C_RegiStatus_t; 

typedef enum {
  C2C_AP_NOT_CONFIG  = 0x00,
  C2C_AP_CONFIGURED  = 0x01,
  C2C_AP_ACVTIVATED  = 0x02,
  C2C_AP_ERROR       = 0xFF
} C2C_APState_t;
  
typedef enum {
  C2C_RET_OK              = 0x00,
  C2C_RET_NOT_SUPPORTED   = 0x01,
  C2C_RET_ERROR           = 0xFF
} C2C_Ret_t;

typedef enum {
  C2C_SEND_OK       = 0x00,  
  C2C_SEND_BUF_FULL = 0x01,
  C2C_SEND_ERROR    = 0xFF
} C2C_SendStatus_t;

  /**
 * \brief  Connection structure
 */



/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void C2C_Power_Off(void);
//MC void C2C_HwResetAndPowerUp(void);
C2C_RegiStatus_t C2C_Init(uint16_t registration_timeout_sec);
C2C_Ret_t        C2C_GetSignalQualityStatus(int32_t *Qvalue);
C2C_Ret_t        C2C_ListOperators(char *Operators);
C2C_Ret_t        C2C_GetCurrentOperator(char *Operator, int32_t Bufsize);
C2C_Ret_t        C2C_ForceOperator(int32_t OperatorCode);

//MC C2C_Ret_t        C2C_ConfigureAP(uint8_t ContextType,
//                                 const char* Apn,
//                                 const char* Username,
//                                 const char* Password,
//                                 uint8_t AuthenticationMode);
//C2C_Ret_t        C2C_Connect(void);
C2C_Ret_t        C2C_Disconnect(void);
C2C_APState_t    C2C_IsConnected(void);

C2C_Ret_t        C2C_GetActiveIpAddresses(char *IPaddr_string, uint8_t* IPaddr_int);


C2C_Ret_t        C2C_Ping(char *host_addr_string, uint16_t count, uint16_t timeout_sec);
C2C_Ret_t        C2C_GetHostAddress(const char *location, uint8_t *ipaddr);

//C2C_Ret_t        C2C_StartClientConnection(uint32_t socket, C2C_Protocol_t type, const char* host_url, uint8_t* ipaddr, uint16_t port, uint16_t local_port);
C2C_Ret_t        C2C_StopClientConnection(uint32_t socket);
C2C_Ret_t        C2C_StartServerConnection(uint32_t socket, C2C_Protocol_t type, uint16_t port, uint16_t local_port);
C2C_Ret_t        C2C_StopServerConnection(uint32_t socket);


C2C_SendStatus_t C2C_SendData(uint32_t socket, uint8_t *pdata, uint16_t Reqlen, uint16_t *SentDatalen, uint32_t Timeout);
C2C_SendStatus_t C2C_SendDataTo(uint32_t socket, uint8_t *pdata, uint16_t Reqlen, uint16_t *SentDatalen, uint32_t Timeout, uint8_t *ipaddr, uint16_t port);
C2C_Ret_t        C2C_ReceiveData(uint32_t socket, uint8_t *pdata, uint16_t Reqlen, uint16_t *RcvDatalen, uint32_t Timeout);
C2C_Ret_t        C2C_ReceiveDataFrom(uint32_t socket, uint8_t *pdata, uint16_t Reqlen, uint16_t *RcvDatalen, uint32_t Timeout, uint8_t *ipaddr, uint16_t *port);
C2C_Ret_t        C2C_RetrieveLastErrorDetails(char *ErrorString);

void             C2C_ResetModule(void);
C2C_Ret_t        C2C_SetModuleDefault(void);
C2C_Ret_t        C2C_ModuleFirmwareUpdate(const char *url);


C2C_Ret_t        C2C_GetModuleID(char *Id);
C2C_Ret_t        C2C_GetModuleFwRevision(char *rev);
C2C_Ret_t        C2C_GetModuleName(char *ModuleName);
C2C_Ret_t        C2C_GetSimId(char *SimId);

#ifdef __cplusplus
}
#endif

#endif /* __C2C_H_ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
