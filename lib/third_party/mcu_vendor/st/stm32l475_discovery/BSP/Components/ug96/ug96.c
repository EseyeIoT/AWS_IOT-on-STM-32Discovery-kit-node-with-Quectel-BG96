/**
  ******************************************************************************
  * @file    ug96.c
  * @author  MCD Application Team
  * @version V0.0.1
  * @date    11-08-2017
  * @brief   Functions to manage the ug96 module (C2C cellular modem).
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
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

#include "ug96.h"
#include "ug96_conf.h"
#include "eseye.h"

#define CHARISHEXNUM(x)                 (((x) >= '0' && (x) <= '9') || \
                                         ((x) >= 'a' && (x) <= 'f') || \
                                         ((x) >= 'A' && (x) <= 'F'))

#define CHARISNUM(x)                    ((x) >= '0' && (x) <= '9')
#define CHAR2NUM(x)                     ((x) - '0')

#define CTRL_Z 26

#define UG96_DBG
// #define UG96_DEEP_DBG
#define UG96_DBG_AT


/* Private variable ---------------------------------------------------------*/
char CmdString[UG96_CMD_SIZE];

/* Exported variable ---------------------------------------------------------*/

/* This should be reworked, _IO should not depend on component */
const UG96_RetKeywords_t ReturnKeywords[] = {
   /* send receive related keywords */
   { RET_SENT,                "SEND OK\r\n" },
   { RET_ARROW,               ">" },
   { RET_READ,                "QIRD: " },
   { RET_SEND,                "QISEND: " },
   { RET_POWERED_DOWN,        "POWERED DOWN\r\n"  },
   { RET_URC_CLOSED,          "closed\""  },
   { RET_URC_RECV,            "recv\""  },
   { RET_URC_IN_FULL,         "incoming full\""  },
   { RET_URC_INCOM,           "incoming\""  },
   { RET_URC_PDPDEACT,        "pdpdeact\""  },
   { RET_URC_DNS,             "dnsgip\""  },
   /* errors keywords */
   { RET_ERROR,               "ERROR\r\n" },
   { RET_CME_ERROR,           "CME ERROR:" },
   //{ RET_CMS_ERROR,           "CMS ERROR:" },
   { RET_BUF_FULL,            "SEND FAIL\r\n" },
   /* set-up keywords */
   { RET_OK,                  "OK\r\n" },
   { RET_OPEN,                "OPEN:" },
   { RET_SIM_READY,           "ready\r\n" },
   { RET_CRLF,                "\r\n" },    /* keep RET_CRLF last !!! */
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Parses and returns number from string.
  * @param  ptr: pointer to string
  * @param  cnt: pointer to the number of parsed digit
  * @retval integer value.
  */
static int32_t  ParseNumber(char* ptr, uint8_t* cnt)
{
    uint8_t minus = 0, i = 0;
    int32_t sum = 0;

    if (*ptr == '-') {                            /* Check for minus character */
        minus = 1;
        ptr++;
        i++;
    }
    while (CHARISNUM(*ptr)) {                     /* Parse number */
        sum = 10 * sum + CHAR2NUM(*ptr);
        ptr++;
        i++;
    }
    if (cnt != NULL) {                            /* Save number of characters used for number */
        *cnt = i;
    }
    if (minus) {                                  /* Minus detected */
        return 0 - sum;
    }
    return sum;                                   /* Return number */
}

/**
  * @brief  Parses and returns QIRQ query response.
  * @param  ptr: pointer to string
  * @param  arr: pointer to IP array
  * @retval None.
  */
static void  ParseQIRD(char* ptr, uint16_t* arr)
{
  uint8_t hexnum = 0, hexcnt;

  while(* ptr) {
    hexcnt = 1;
    if(*ptr != ',')
    {
      arr[hexnum++] = (uint16_t) ParseNumber(ptr, &hexcnt);
    }
    ptr = ptr + hexcnt;
    if((*ptr == '\r') || (hexnum == 3))
    {
      return;
    }
  }
}

/**
  * @brief  Parses and returns .
  * @param  ptr: pointer to string
  * @retval The number of unread data in modem TX buffer.
  */
static void  ParseQISEND(char* ptr, uint32_t* unackedbytes)
{
  uint8_t hexnum = 0, hexcnt;
  uint32_t temp = 0;

  /* QISEND Response contains 3 kind of information
   * <total_send_length>,<ackedbytes>,<unackedbytes>
   * we're only interested to the 3rd one, unackedbytes for now */
  while(*ptr) {
    hexcnt = 1;
    if(*ptr != ',')
    {
      temp = (uint32_t) ParseNumber(ptr, &hexcnt);
      /* Count up the number we've retrieved */
      hexnum++;
      if (hexnum == 3)
      {
        *unackedbytes  = temp;
      }
    }
    ptr = ptr + hexcnt;
    if((*ptr == '\r') || (hexnum == 3))
    {
      return;
    }
  }
}

/**
  * @brief  Parses and returns IP address.
  * @param  ptr: pointer to string
  * @param  arr: pointer to IP array
  * @retval None.
  */
static void  ParseIP(char* ptr, uint8_t* arr)
{
  uint8_t hexnum = 0, hexcnt;

  while(* ptr) {
    hexcnt = 1;
    if(*ptr != '.')
    {
      arr[hexnum++] = ParseNumber(ptr, &hexcnt);
    }
    ptr = ptr + hexcnt;
    if(*ptr == '"')
    {
      return;
    }
  }
}

/**
 * @brief   Return the integer difference between 'init + timeout' and 'now'.
 *          The implementation is robust to uint32_t overflows.
 * @param   In:   init      Reference index.
 * @param   In:   now       Current index.
 * @param   In:   timeout   Target index.
 * @retval  Number of ms from now to target (init + timeout).
 */
static int32_t TimeLeftFromExpiration(uint32_t init, uint32_t now, uint32_t timeout)
{
  int32_t ret = 0;
  uint32_t wrap_end = 0;

  if (now < init)
  { /* Timer wrap-around detected */
    wrap_end = UINT32_MAX - init;
  }
  ret = wrap_end - (now - init) + timeout;

  return ret;
}

/**
  * @brief  Retrieve Data from the C2C module over the UART interface.
  *         This function receives data from the  C2C module, the
  *         data is fetched from a ring buffer that is asynchronously and continuously
            filled with the received data.
  * @param  Obj: pointer to module handle
  * @param  pData: a buffer inside which the data will be read.
  * @param  Length: Size of the data to receive.
  * @param  ScanVals: when param Length = 0 : values to be retrieved in the coming data in order to exit.
  * @retval int32_t: if param Length = 0 : the actual RET_CODE found,
                     if param Length > 0 : the actual data size that has been received
                     if error (timeout): return -1 (UG96_RETURN_RETRIEVE_ERROR).
  */
static int32_t AT_RetrieveData(Ug96Object_t *Obj, uint8_t* pData, uint16_t Length, uint32_t ScanVals, uint32_t Timeout)
{
  uint32_t tickstart = Obj->GetTickCb();
  int16_t ReadData = 0;
  uint16_t x;
  uint16_t index[NUM_RESPONSES];
  uint16_t lens[NUM_RESPONSES];
  uint16_t  pos;
  uint8_t c;
  int32_t  min_requested_time;

  min_requested_time = 100;
  if (Timeout < min_requested_time)       /* UART speed 115200 bits per sec */
  {
     Timeout = min_requested_time;
#ifdef UG96_DBG
     //configPRINTF(("UART_C2C: Timeout forced to respect UART speed %d: %ld\r\n", UG96_DEFAULT_BAUDRATE, min_requested_time));
#endif
  }

  for (x = 0; x < NUM_RESPONSES; x++)
  {
    index[x] = 0;
    lens[x] = strlen(ReturnKeywords[x].retstr);
  }

  if ((Length == 0) && (ScanVals == RET_NONE))
  {
     return 0;  /* to avoid waiting a RET_VAL in case the parsed_lenth of payload is zero */
                /* but no code needs to be retrieved */
  }

  memset(Obj->CmdResp, 0, UG96_CMD_SIZE);

  while (TimeLeftFromExpiration(tickstart, Obj->GetTickCb(), Timeout) > 0)
  {
    if(Obj->fops.IO_ReceiveOne(&c) == 0)   /* Receive one sample from UART */
    {
      /* serial data available, so return data to user */
      pData[ReadData++] = c;

      if (Length == 0)
      {
        /* Check whether we hit an ESP return values */
        for(x = 0; x < NUM_RESPONSES; x++)
        {
          if (c != ReturnKeywords[x].retstr[index[x]])
          {
            index[x] = 0;
          }

          if (c == ReturnKeywords[x].retstr[index[x]])
          {
            pos = ++(index[x]);
            if( pos >= lens[x])
            {
              if (ScanVals & ReturnKeywords[x].retval)
              {
                return ReturnKeywords[x].retval;
              }
            }
          }
        }
      }
      else  /* end (Length > 0) */
      {
        if (ReadData < Length)
        {
          /* nothing to do except keep reading in the while loop */
        }
        else /* ReadData >= Length */
        {
          return ReadData;
        }
      } /* end (Length == 0) */
    }
    else
    {
    	vTaskDelay(pdMS_TO_TICKS(1));
    }
  }
  if ((Length > 0) && (ReadData > 0))
  {
#ifdef UG96_DBG
    configPRINTF(("UART_C2C: Warning: timeout occurred before all data was read (%d/%u)\r\n", ReadData, Length));
#endif
    return ReadData;
  }
#ifdef UG96_DEEP_DBG
    configPRINTF(("UART_C2C: RET_CODE not found or non a single byte has been read at all\r\n"));
#endif
  return UG96_RETURN_RETRIEVE_ERROR;
}

/**
  * @brief  Execute AT command.
  * @param  Obj: pointer to module handle
  * @param  cmd: pointer to command string
  * @param  resp: expected response
  * @retval Operation Status: OK or ERROR.
  */
static int32_t  AT_ExecuteCommand(Ug96Object_t *Obj, uint32_t timeout, uint8_t* cmd, uint32_t resp)
{
  int32_t ret = UG96_RETURN_SEND_ERROR;

  if (timeout == 0)
  {
    timeout = UG96_TOUT_300;
  }
#ifdef UG96_DBG_AT
  configPRINTF(("AT Request: %s\r\n", cmd));
#endif
  if(Obj->fops.IO_Send(cmd, strlen((char*)cmd)) >= 0)
  {
    ret = (AT_RetrieveData(Obj, Obj->CmdResp, 0, resp, timeout));
    if (ret < 0)
    {
#ifdef UG96_DBG
      //configPRINTF(("UG96 AT_ExecuteCommand() rcv TIMEOUT ret=%ld: %s\r\n", ret, cmd));
#endif
    }
#ifdef UG96_DBG_AT
    else
    {
      configPRINTF(("AT Response: %s\r\n", Obj->CmdResp));
    }
#endif
  }
  else
  {
#ifdef UG96_DBG
    configPRINTF(("UG96 AT_ExecuteCommand() send ERROR: %s\r\n", cmd));
#endif
  }
  return ret;
}

/**
  * @brief  Execute AT command with data.
  * @param  Obj: pointer to module handle
  * @param  pdata: pointer to returned data
  * @param  len: binary data length
  * @param  timeout: waiting that the modem confirms confirm that send command has been executed (SEND OK)
  * @retval Operation Status.
  */
static UG96_SendRet_t  AT_RequestSendData(Ug96Object_t *Obj, uint8_t *pdata, uint16_t len, uint32_t timeout)
{
  int32_t confirm;

  if(Obj->fops.IO_Send(pdata, len) >= 0)
  {
      confirm = AT_RetrieveData(Obj, Obj->CmdResp, 0, (RET_SENT | RET_BUF_FULL | RET_ERROR), timeout);
      return (UG96_SendRet_t) confirm;
  }
  return UG96_SEND_RET_UART_FAIL; /* UART error to transmit */
}

/**
  * @brief  Retrieve URC header and decode it
  * @param  Obj: pointer to module handle
  * @param  Timeout : ms
  * @param  ParseLength : pointer to return the length parsed from URC info
  * @param  AccMode : currently only UG96_BUFFER_MODE is used and tested
  * @retval Operation Status.
  */
static int32_t  AT_RetrieveUrc(Ug96Object_t *Obj, uint32_t Timeout, uint16_t *ParseLength, UG96_AccessMode_t AccMode)
{
  int32_t check_resp, ret;
  uint32_t expected_resp;
  uint8_t parse_ret;
  uint8_t parse_count, ip_count;
  uint32_t count = 0;

  expected_resp = RET_URC_CLOSED | RET_URC_RECV | RET_URC_IN_FULL | RET_URC_INCOM | RET_URC_PDPDEACT | RET_URC_DNS;
  check_resp =  AT_RetrieveData(Obj, Obj->CmdResp, 0, expected_resp, Timeout);

  switch (check_resp) {
    case RET_URC_CLOSED:
      AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_CRLF, UG96_TOUT_SHORT);
      parse_ret = ParseNumber((char *) Obj->CmdResp+1, &parse_count);
      break;
    case RET_URC_RECV:
      /* retrieve contextID */
      ret = AT_RetrieveData(Obj, Obj->CmdResp, 3, RET_NONE, UG96_TOUT_SHORT);
      if (ret > 0)
      {
        parse_ret = ParseNumber((char *) Obj->CmdResp+1, &parse_count);
        if (parse_count == 2)
        { /* read next comma */
          AT_RetrieveData(Obj, Obj->CmdResp, 1, RET_NONE, UG96_TOUT_SHORT);
        }
      }
      if (AccMode == UG96_BUFFER_MODE)
      {
          AT_RetrieveData(Obj, Obj->CmdResp, 1, RET_NONE, UG96_TOUT_SHORT);
      }
      else
      {
        for( ;count < 6; count++)
        {
          if (ret < 0)
          {
            break;
          }
          ret = AT_RetrieveData(Obj, &Obj->CmdResp[count], 1, RET_NONE, UG96_TOUT_SHORT);
          if(ret == 1)
          {
            if(Obj->CmdResp[count] == '\n')
            {
              *ParseLength = (uint16_t) ParseNumber((char *) Obj->CmdResp, &parse_count);
              break;
            }
          }
        }
      }
      break;

    case RET_URC_IN_FULL:
      /* nothing to be done */
      break;

    case RET_URC_INCOM:
      /* TBD:                     to be implemented for SERVER MODE*/
      break;

    case RET_URC_PDPDEACT:
      AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_CRLF, UG96_TOUT_SHORT);
      parse_ret = ParseNumber((char *) Obj->CmdResp+1, &parse_count);
      break;

    case RET_URC_DNS:
      AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_CRLF, Timeout);
      parse_ret = ParseNumber((char *) Obj->CmdResp+1, &parse_count);
      if (parse_ret == 0) /* means no errors */
      {
        ip_count = ParseNumber((char *) Obj->CmdResp+3, &parse_count);
        for (count = 0; count < ip_count; count++)
        {
          expected_resp = RET_URC_DNS;
          check_resp =  AT_RetrieveData(Obj, Obj->CmdResp, 0, expected_resp, Timeout);
          AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_CRLF, Timeout);
        }
      }
      else
      {
        check_resp = -1;
      }
      break;

    default :
      check_resp = -1;
      break;
  }

  return check_resp;
}

/**
  * @brief  Synchronize the modem uart with the STM32 uart (autobauding)
  * @param  Obj: pointer to module handle
  * @retval Operation status.
  */
static int32_t  AT_Synchro(Ug96Object_t *Obj)
{
  int32_t ret = UG96_RETURN_SEND_ERROR;
  int8_t atSync = 0;
  uint32_t tickstart;

  /* Init tickstart for timeout management */
  tickstart = Obj->GetTickCb();

  /* Start AT SYNC: Send AT every 500ms,
  if receive OK, SYNC success,
  if no OK return after sending AT 10 times, SYNC fail */
  do {
    if (TimeLeftFromExpiration(tickstart, Obj->GetTickCb(), UG96_TOUT_ATSYNC) < 0) {
      ret = AT_ExecuteCommand(Obj, UG96_TOUT_SHORT, (uint8_t *)"AT\r\n", RET_OK | RET_ERROR);
      atSync++;
      tickstart = Obj->GetTickCb();
    }
  }
  while ((atSync < 10) && (ret != RET_OK));

  return ret;
}

/* --------------------------------------------------------------------------*/
/* --- Public functions -----------------------------------------------------*/
/* --------------------------------------------------------------------------*/

/**
  * @brief  Register UG96 BusIO external functions.
  * @param  Obj: pointer to module handle
  * @retval Operation Status.
  */
UG96_Return_t  UG96_RegisterBusIO(Ug96Object_t *Obj, IO_Init_Func        IO_Init,
                                                     IO_DeInit_Func      IO_DeInit,
                                                     IO_Baudrate_Func    IO_Baudrate,
                                                     IO_Send_Func        IO_Send,
                                                     IO_ReceiveOne_Func  IO_ReceiveOne,
                                                     IO_Flush_Func       IO_Flush)
{
  if(!Obj || !IO_Init || !IO_DeInit || !IO_Baudrate || !IO_Send || !IO_ReceiveOne || !IO_Flush)
  {
    return UG96_RETURN_ERROR;
  }

  Obj->fops.IO_Init = IO_Init;
  Obj->fops.IO_DeInit = IO_DeInit;
  Obj->fops.IO_Baudrate = IO_Baudrate;
  Obj->fops.IO_Send = IO_Send;
  Obj->fops.IO_ReceiveOne = IO_ReceiveOne;
  Obj->fops.IO_FlushBuffer = IO_Flush;

  return UG96_RETURN_OK;
}

/**
  * @brief  Shut down the module.
  * @retval Operation Status.
  */
UG96_Return_t UG96_PowerDown(Ug96Object_t *Obj)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;

  if ( RET_OK == (UG96_Return_t) AT_ExecuteCommand(Obj,                        \
                                                  UG96_TOUT_300,               \
                                                  (uint8_t *)"AT+QPOWD=1\r\n", \
                                                  RET_OK | RET_ERROR | RET_CME_ERROR) )
  {
    configPRINTF(("Please wait, disconnecting and saving data. It may last until 60 s\r\n"));

    /* expect for the "POWERED DOWN" */
    /* The maximum time for network log-off is 60 seconds */
    if ( AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_POWERED_DOWN, UG96_TOUT_60000) > 0)
    {
      configPRINTF(("Modem is entered in power down\r\n"));
      ret = UG96_RETURN_OK;
    }
  }
  return ret;
}

/**
  * @brief  Initialize UG96 module.
  * @param  Obj: pointer to module handle
  * @retval Operation Status.
  */
UG96_InitRet_t  UG96_Init(Ug96Object_t *Obj)
{
  UG96_InitRet_t fret = UG96_INIT_OTHER_ERR;
  int32_t ret = RET_ERROR;
  int8_t i;
  char *align_ptr;
  uint8_t parse_count;
  uint32_t tickstart;

  Obj->APsActive = 0;
  for (i = 0; i < UG96_MAX_SOCKETS; i++)
  {
    Obj->SocketInfo[i].Type = UG96_TCP_CONNECTION;
    Obj->SocketInfo[i].AccessMode = UG96_BUFFER_MODE;
    Obj->SocketInfo[i].ComulatedQirdData = 0;
    Obj->SocketInfo[i].HaveReadLength = 0;
    Obj->SocketInfo[i].UnreadLength = 0;
  }

  Obj->fops.IO_FlushBuffer();  /* Flush Uart intermediate buffer */

  if (Obj->fops.IO_Init() == 0) /* configure and initialize UART */
  {
    ret = AT_Synchro(Obj);

    if(ret != RET_OK)
    {
      configPRINTF(("Fail to AT SYNC, after several attempts\r\n"));

      fret = UG96_INIT_RET_AT_ERR; /* if does not respond to AT command set specific return status */
    }
    else
    {
      /* Retrieve Quectel Factory Default values */
      ret = UG96_ResetToFactoryDefault(Obj);
      /* Retrieve Quectel UART baud rate and flow control*/
      /* If not aligned to the UART of MCU (_io.h), already previous AT command will fail */
      ret = ret | UG96_GetUARTConfig(Obj, &Obj->UART_Config);

      /* Use ATV1 to set the response format */
      ret = ret | AT_ExecuteCommand(Obj, UG96_TOUT_SHORT, (uint8_t *)"ATV1\r\n", RET_OK | RET_ERROR);
      /* Use ATE1 to enable or  ATE0 to disable echo mode */
      ret = ret | AT_ExecuteCommand(Obj, UG96_TOUT_SHORT, (uint8_t *)"ATE0\r\n", RET_OK | RET_ERROR);
       /* Use AT+CMEE=1 to enable result code and use "integer" values */
      ret = ret | AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *)"AT+CMEE=1\r\n", RET_OK | RET_ERROR);
    }

    /* retrieve SIM info */
    if(ret == RET_OK)
    {

      /* A tempo is required to get the SIM ready. Set to 2000 ms */
      tickstart = Obj->GetTickCb();
      while((Obj->GetTickCb() - tickstart) < 2000)
      {
      }

      ret = AT_ExecuteCommand(Obj, UG96_TOUT_5000, (uint8_t *)"AT+CPIN?\r\n", RET_OK | RET_ERROR | RET_CME_ERROR);

      if (RET_OK == ret)
      {
        align_ptr = strstr((char *) Obj->CmdResp,"+CPIN: READY");
        if ( NULL != align_ptr )
        {
          Obj->SimInfo.SimStatus = UG96_SIM_READY;

    	  fret = UG96_INIT_RET_OK;
        }
      }
      else
      {
        if (RET_CME_ERROR == ret)
        {
          AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_CRLF, UG96_TOUT_SHORT);
          Obj->SimInfo.SimStatus = (UG96_SIMState_t) ParseNumber((char *) Obj->CmdResp+1, &parse_count);
          fret = UG96_INIT_RET_SIM_ERR;
        }
      }
    }
#ifdef USE_BG96

    /* MC the next three commands set the correct band */
    if(ret == RET_OK)
	{
		ret = AT_ExecuteCommand(Obj, UG96_TOUT_15000, (uint8_t *)"AT+QCFG=\"roamservice\",2,1\r\n", RET_OK | RET_ERROR | RET_CME_ERROR);
		if (RET_OK != ret)
		{
			ret = UG96_INIT_OTHER_ERR;
		}
        /* Use AT+GSN to query the IMEI (International Mobile Equipment Identity) of module */
        ret = AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *)"AT+GSN\r\n", RET_OK | RET_ERROR);
        if (ret == RET_OK)
        {
          align_ptr = strstr((char *) Obj->CmdResp,"\r\n") + 2;
          strncpy((char *)Obj->Imei, align_ptr, UG96_IMEI_SIZE -1 );
        }
        /* Use AT+QCCID to query the ICCID of the SIM  */
        ret = AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *)"AT+QCCID\r\n", RET_OK | RET_ERROR);
        if (RET_OK != ret)
        {
        	ret = UG96_INIT_OTHER_ERR;
        }
        /* Use AT+CIMI to query the IMSI (International Mobile Subscriber Identity) of SIM */
        ret = AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *)"AT+CIMI\r\n", RET_OK | RET_ERROR | RET_CME_ERROR);
        if (RET_OK != ret)
        {
        	ret = UG96_INIT_OTHER_ERR;
        }

	}
    if(ret == RET_OK)
    {
    	ret = AT_ExecuteCommand(Obj, UG96_TOUT_15000, (uint8_t *)"AT+QCFG=\"nwscanseq\",020103,1\r\n", RET_OK | RET_ERROR | RET_CME_ERROR);
    	if (RET_OK != ret)
    	{
    		ret = UG96_INIT_OTHER_ERR;
    	}
    }
    if(ret == RET_OK)
    {
    	ret = AT_ExecuteCommand(Obj, UG96_TOUT_15000, (uint8_t *)"AT+QCFG=\"nwscanmode\",0,1\r\n", RET_OK | RET_ERROR | RET_CME_ERROR);
    	if (RET_OK != ret)
    	{
    		ret = UG96_INIT_OTHER_ERR;
    	}
    }
    if(ret == RET_OK)
    {
    	ret = AT_ExecuteCommand(Obj, UG96_TOUT_15000, (uint8_t *)"AT+QCFG=\"band\", 0f, 400A0E189F, A0E189F, 1\r\n", RET_OK | RET_ERROR | RET_CME_ERROR);
    	if (RET_OK != ret)
    	{
    		ret = UG96_INIT_OTHER_ERR;
    	}
    }
    /* Set the radio ON with the full functionality in the modem */
    ret = AT_ExecuteCommand(Obj, UG96_TOUT_15000, (uint8_t *)"AT+CFUN=1\r\n", RET_OK | RET_ERROR | RET_CME_ERROR);
    if (RET_OK != ret)
    {
      fret = UG96_INIT_OTHER_ERR;
    }
    ret = AT_ExecuteCommand(Obj, UG96_TOUT_15000, (uint8_t *)"AT+CREG=1\r\n", RET_OK | RET_ERROR | RET_CME_ERROR);
	if (RET_OK != ret)
	{
	  fret = UG96_INIT_OTHER_ERR;
	}

#endif
  }
  else
  {
     fret = UG96_INIT_RET_IO_ERR;
  }

  return fret;
}

/**
  * @brief  Get Signal Quality value
  * @param  Obj: pointer to module handle
  * @retval Operation status.
  */
UG96_Return_t  UG96_GetSignalQualityStatus(Ug96Object_t *Obj, int32_t *Qvalue)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;
  uint8_t parse_count;
  char *align_ptr;

  ret = (UG96_Return_t) AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *)"AT+CSQ\r\n", RET_OK | RET_ERROR | RET_CME_ERROR);
  if (RET_OK == ret)
  {
    align_ptr = strstr((char *) Obj->CmdResp,"+CSQ:") + sizeof("+CSQ:");
    *Qvalue = ParseNumber(align_ptr, &parse_count);
  }
  return ret;
}

/**
  * @brief  Attach the MT to the packet domain service
  * @param  Obj: pointer to module handle
  * @retval Operation status.
  */
UG96_Return_t  UG96_PSAttach(Ug96Object_t *Obj)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;

  if (Obj->SimInfo.SimStatus == UG96_SIM_READY)
  {
    if (RET_OK != AT_ExecuteCommand(Obj, UG96_TOUT_75000, (uint8_t *)"AT+CGATT=1\r\n", RET_OK | RET_ERROR | RET_CME_ERROR) )
    {
      AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_CRLF, UG96_TOUT_SHORT);
      ret = UG96_RETURN_ERROR;
    }
    else
    {
      ret = UG96_RETURN_OK;
    }
  }
  return ret;
}

/**
  * @brief  Force an automatic PLMN selection
  * @param  Obj: pointer to module handle
  * @retval Operation status.
  */
UG96_Return_t  UG96_AutomaticPlmnSelection(Ug96Object_t *Obj)
{
  UG96_Return_t ret;

  if (RET_OK != AT_ExecuteCommand(Obj, UG96_TOUT_180000, (uint8_t *)"AT+COPS=0\r\n", RET_OK | RET_ERROR | RET_CME_ERROR) )
  {
    AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_CRLF, UG96_TOUT_SHORT);
    ret = UG96_RETURN_ERROR;
  }
  else
  {
    ret = UG96_RETURN_OK;
  }
  return ret;
}

UG96_Return_t  UG96_NetworkSearch(Ug96Object_t *Obj)
{
  UG96_Return_t ret;

  if (RET_OK != AT_ExecuteCommand(Obj, UG96_TOUT_180000, (uint8_t *)"AT+COPS=?\r\n", RET_OK | RET_ERROR | RET_CME_ERROR) )
  {
    AT_RetrieveData(Obj, Obj->CmdResp, 256, RET_CRLF, UG96_TOUT_SHORT);
    ret = UG96_RETURN_ERROR;
  }
  else
  {
    ret = UG96_RETURN_OK;
  }
  return ret;
}

UG96_Return_t  UG96_SetFullFunctionality(Ug96Object_t *Obj)
{
  UG96_Return_t ret;

  if (RET_OK != AT_ExecuteCommand(Obj, UG96_TOUT_15000, (uint8_t *)"AT+CFUN=1\r\n", RET_OK | RET_ERROR | RET_CME_ERROR) )
  {
    AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_CRLF, UG96_TOUT_SHORT);
    ret = UG96_RETURN_ERROR;
  }
  else
  {
    ret = UG96_RETURN_OK;
  }
  return ret;
}

/**
  * @brief  Get Circuit Switch Registration Status
  * @param  Obj: pointer to module handle
  * @retval Registration Status.
  */
UG96_NetworkRegistrationState_t  UG96_GetCsNetworkRegistrationStatus(Ug96Object_t *Obj)
{
	int8_t received_string[3];
	int16_t val;
	UG96_NetworkRegistrationState_t ret = UG96_NRS_ERROR;
	char *align_ptr;
	char *creg_ptr;

	if (Obj->SimInfo.SimStatus == UG96_SIM_READY)
	{
		while(ret != UG96_NRS_HOME_NETWORK && ret != UG96_NRS_ROAMING)
		{
			if (RET_OK == AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *)"AT+CREG?\r\n", RET_OK | RET_ERROR | RET_CME_ERROR))
			{
				creg_ptr = strstr((char *) Obj->CmdResp,"+CREG:");

				/* +CREG: is in the response*/
				if (NULL != creg_ptr)
				{
					/* search for <stat> is in the response */
					align_ptr = creg_ptr + sizeof("+CREG:");
					strncpy((char *)received_string,  align_ptr+2, 1);
					strncpy((char *)received_string+1,  "\r\n", 2);
					val = atoi((char *)received_string);
					ret = (UG96_NetworkRegistrationState_t) val;
				}
				vTaskDelay(pdMS_TO_TICKS(1000));
			}
		}
	}
	return ret;
}

/**
  * @brief  Get Packet Switch Registration Status
  * @param  Obj: pointer to module handle
  * @retval Registration Status.
  */
UG96_NetworkRegistrationState_t  UG96_GetPsNetworkRegistrationStatus(Ug96Object_t *Obj)
{
  int8_t received_string[3];
  int16_t val;
  UG96_NetworkRegistrationState_t ret = UG96_NRS_ERROR;
  char *align_ptr;
  char *cgreg_ptr;

  if (Obj->SimInfo.SimStatus == UG96_SIM_READY)
  {
    if (RET_OK == AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *)"AT+CGREG?\r\n", RET_OK | RET_ERROR | RET_CME_ERROR))
    {
      cgreg_ptr = strstr((char *) Obj->CmdResp,"+CGREG:");

      /* +CGREG: is in the response*/
      if (NULL != cgreg_ptr)
      {
        align_ptr = cgreg_ptr + sizeof("+CGREG:");
        strncpy((char *)received_string,  align_ptr+2, 1);
        strncpy((char *)received_string+1,  "\r\n", 2);
        val = atoi((char *)received_string);
        ret = (UG96_NetworkRegistrationState_t) val;
      }
    }
  }
  return ret;
}

/**
  * @brief  Force registration to specific Network Operator (by operator code).
  * @param  Obj: pointer to module handle
  * @param  OperatorCode: http://www.imei.info/operator-codes/
  * @retval Operation Status.
  */
UG96_Return_t  UG96_ForceOperator(Ug96Object_t *Obj, int32_t OperatorCode)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;

  snprintf(CmdString, 24,"AT+COPS=1,2,\"%ld\"\r\n", OperatorCode);
  ret = (UG96_Return_t) AT_ExecuteCommand(Obj, UG96_TOUT_180000, (uint8_t *)CmdString, RET_OK | RET_ERROR | RET_CME_ERROR);
  return ret;
}


/**
  * @brief  Reset To factory defaults.
  * @param  Obj: pointer to module handle
  * @retval Operation Status.
  */
UG96_Return_t UG96_ResetToFactoryDefault(Ug96Object_t *Obj)
{
  UG96_Return_t ret ;

  ret = (UG96_Return_t) AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t*) "AT&F0\r\n", RET_OK | RET_ERROR);
  return ret;
}


/**
  * @brief  Set UART Configuration on the Quectel modem and on STM32.
  * @param  Obj: pointer to module handle
  * @param  pconf: pointer to UART config structure
  * @retval Operation Status.
  */
UG96_Return_t  UG96_SetUARTBaudrate(Ug96Object_t *Obj, UG96_UARTConfig_t *pconf)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;

  /* change the UART baudrate on UG96 module */
  snprintf(CmdString, 17,"AT+IPR=%lu\r\n", pconf->BaudRate);
  ret = (UG96_Return_t) AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *)CmdString, RET_OK | RET_ERROR);

  /* change the UART baudrate on STM32 microcontroller accordingly */
  Obj->fops.IO_Baudrate(pconf->BaudRate);

  return ret;
}

/**
  * @brief  Get UART Configuration.
  * @param  Obj: pointer to module handle
  * @param  pconf: pointer to UART config structure
  * @retval Operation Status.
  */
UG96_Return_t  UG96_GetUARTConfig(Ug96Object_t *Obj, UG96_UARTConfig_t *pconf)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;
  char *align_ptr;
  uint8_t rts, cts;

  ret = (UG96_Return_t) AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *)"AT+IPR?\r\n", RET_OK | RET_ERROR);
  if (ret == RET_OK)
  {
    align_ptr = strstr((char *) Obj->CmdResp,"+IPR:") + sizeof("+IPR:");
    Obj->UART_Config.BaudRate = ParseNumber(align_ptr, NULL);
  }

  ret = (UG96_Return_t) AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *)"AT+IFC?\r\n", RET_OK | RET_ERROR);
  if (ret == RET_OK)
  {
    align_ptr = strstr((char *) Obj->CmdResp,"+IFC:") + sizeof("+IPR:");
    rts = ParseNumber(align_ptr, NULL);
    cts = ParseNumber(align_ptr+2, NULL);

    if (rts == 2)
    {
      if (cts == 2)
      {
        pconf->FlowControl = UG96_UART_FLW_CTL_RTS_CTS;
      }
      else
      {
        pconf->FlowControl = UG96_UART_FLW_CTL_RTS;
      }
    }
    else
    {
      if (cts == 2)
      {
        pconf->FlowControl = UG96_UART_FLW_CTL_CTS;
      }
      else
      {
        pconf->FlowControl = UG96_UART_FLW_CTL_NONE;
      }
    }
  }

  return ret;
}


/**
  * @brief  Retrieve last IP error code
  * @param  Obj: pointer to module handle
  * @param  error_string:
  * @param  error_code
  * @retval Operation Status.
  */
UG96_Return_t  UG96_RetrieveLastErrorDetails(Ug96Object_t *Obj, char *error_string)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;
  char *align_ptr;

  ret = (UG96_Return_t) AT_ExecuteCommand(Obj, UG96_TOUT_SHORT, (uint8_t *)"AT+QIGETERROR\r\n", RET_OK | RET_ERROR);
  align_ptr = strstr((char *) Obj->CmdResp,"+QIGETERROR:") + sizeof("+QIGETERROR:");
  strncpy((char *)error_string,  align_ptr, UG96_ERROR_STRING_SIZE);
  return ret;
}

/**
  * @brief  Register UG96 Tick external cb functions to be provided by application (e.g. xTaskGetTickCount)
  * @param  Obj: pointer to module handle
  * @param  GetTickCb: pointer to callback function that should provide a Timer Tick in ms
  * @retval Operation Status.
  */
UG96_Return_t  UG96_RegisterTickCb(Ug96Object_t *Obj, App_GetTickCb_Func  GetTickCb)
{
  if(!Obj || !GetTickCb)
  {
    return UG96_RETURN_ERROR;
  }

  Obj->GetTickCb = GetTickCb;

  return UG96_RETURN_OK;
}

/* ==== AP Connection ==== */

/**
  * @brief  Configure a PDP Access point.
  * @param  Obj: pointer to module handle
  * @param  ContextID : range is 1-20
  * @param  Apn : access point name
  * @param  Username : Got IP Address
  * @param  Password : Network IP mask
  * @param  AuthenticationMode : 0: none, 1: PAP
  * @retval Operation Status.
  */
UG96_Return_t  UG96_ConfigureAP(Ug96Object_t *Obj,
                                UG96_APConfig_t *ApConfig)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;

  snprintf(CmdString,UG96_CMD_SIZE,"AT+QICSGP=%d,1,\"%s\",\"%s\",\"%s\",%d\r\n", ApConfig->ContextID, ApConfig->ApnString, ApConfig->Username, ApConfig->Password, ApConfig->Authentication);
  ret = (UG96_Return_t) AT_ExecuteCommand(Obj, UG96_TOUT_SHORT, (uint8_t *)CmdString, RET_OK | RET_ERROR);
  if(ret == UG96_RETURN_OK)
  {
     Obj->APContextState[ApConfig->ContextID-1] = UG96_AP_CONFIGURED;
  }
  return ret;
}

/**
  * @brief  Join a PDP Access point.
  * @param  Obj: pointer to module handle
  * @param  ContextID : range is 1-20 (max three can be connected simultaneously)
  * @retval Operation Status.
  */
UG96_Return_t  UG96_Activate(Ug96Object_t *Obj, uint8_t ContextID)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;

  if (Obj->APContextState[ContextID-1] == UG96_AP_CONFIGURED)
  {
    if (Obj->APsActive <3 )
    {
      snprintf(CmdString, 24,"AT+QIACT=%d\r\n", ContextID);
      ret = (UG96_Return_t) AT_ExecuteCommand(Obj, UG96_TOUT_150000, (uint8_t *)CmdString, RET_OK | RET_ERROR);
      if(ret == UG96_RETURN_OK)
      {
         Obj->APContextState[ContextID-1] = UG96_AP_ACVTIVATED;
         Obj->APsActive++;
      }
    }
  }
#ifdef UG96_DBG
  configPRINTF(("UG96_Activate() PDP Access point, ret value: %d\r\n", ret));
#endif
  return ret;
}

/**
  * @brief  Leave a PDP Access point.
  * @param  Obj: pointer to module handle
  * @param  ContextID : range is 1-20 (max three are connected simultaneously)
  * @retval Operation Status.
  */
UG96_Return_t  UG96_Deactivate(Ug96Object_t *Obj, uint8_t ContextID)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;

  snprintf(CmdString, 24,"AT+QIDEACT=%d\r\n", ContextID);
  ret = (UG96_Return_t) AT_ExecuteCommand(Obj, UG96_TOUT_40000, (uint8_t *)CmdString, RET_OK | RET_ERROR);
  if(ret == UG96_RETURN_OK)
  {
     Obj->APContextState[ContextID-1] = UG96_AP_CONFIGURED;
     Obj->APsActive--;
  }
#ifdef UG96_DBG
  configPRINTF(("UG96_Deactivate() PDP Access point, ret value: %d\r\n", ret));
#endif
  return ret;
}

/**
  * @brief  Check whether the contextID is connected to an access point.
  * @retval Operation Status.
  */
UG96_APState_t  UG96_IsActivated(Ug96Object_t *Obj, uint8_t ContextID)
{
  return (UG96_APState_t) Obj->APContextState[ContextID-1];
}

/**
  * @brief  Get the list of the current activated context and its IP addresses
  * @param  Obj: pointer to module handle
  * @param  IPaddr_string: pointer where to retrieve the string with all active IP info
  * @param  IPaddr_int: pointer where to retrieve the first active IP address in int_array[] format
  * @retval Operation Status.
  */
UG96_Return_t  UG96_GetActiveIpAddresses(Ug96Object_t *Obj, char *IPaddr_string, uint8_t* IPaddr_int)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;
  int32_t cmdret;
  char *align_ptr;
  uint8_t exit =0;

  cmdret = AT_ExecuteCommand(Obj, UG96_TOUT_150000, (uint8_t *)"AT+QIACT?\r\n", RET_OK | RET_ERROR);
  if (cmdret == RET_OK)
  {
    align_ptr = strstr((char *) Obj->CmdResp,"+QIACT:") + sizeof("+QIACT:");
    strncpy((char *)IPaddr_string,  align_ptr, 40);
    align_ptr = IPaddr_string+2;
    while(!exit)
    {   /* find where numbers start because it is not always same position */
      align_ptr++;
      if(*align_ptr == '"')
      {
        exit =1;
      }
      if (align_ptr > (IPaddr_string + 40))
      {
        ret = UG96_RETURN_ERROR;
        break;
      }
    }
    ParseIP(align_ptr+1, IPaddr_int);
    ret = UG96_RETURN_OK;
  }
  return ret;
}

#if (UG96_USE_PING == 1)
/**
  * @brief  Test the Internet Protocol reachability of a host
  * @param  Obj: pointer to module handle
  * @param  ContextID : range is 1-20 (max three are connected simultaneously)
  * @param  host_addr_string: domain name (e.g. www.amazon.com) or dotted decimal IP addr
  * @param  count: PING repetitions (default 4) (max 10)
  * @param  rep_delay_sec: timeout for each repetition in seconds
  * @retval Operation Status.
  */
UG96_Return_t  UG96_Ping(Ug96Object_t *Obj, uint8_t ContextID, char *host_addr_string, uint16_t count, uint16_t rep_delay_sec)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;

  if (count > 10)
  {
    count = 10;
  }
  snprintf(CmdString, UG96_CMD_SIZE,"AT+QPING=%d,\"%s\",%d,%u\r\n", ContextID, host_addr_string, rep_delay_sec, count);
  ret = (UG96_Return_t) AT_ExecuteCommand(Obj, UG96_TOUT_150000, (uint8_t *)CmdString, RET_OK | RET_ERROR);
  return ret;
}
#endif

/* ==== Client connection and communication ==== */

/**
  * @brief  Get the last IP addresses associated to the host name via Google DNS service
  * @param  Obj: pointer to module handle
  * @param  ContextID : range is 1-20 (max three are connected simultaneously)
  * @param  IPaddr_string: host name (e.g. www.host.com)
  * @param  IPaddr_int: pointer where to retrieve the first active IP address in int_array[] format
  * @retval Operation Status.
  */
UG96_Return_t  UG96_DNS_LookUp(Ug96Object_t *Obj, uint8_t ContextID, const char *IPaddr_string, uint8_t* IPaddr_int)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;
  int32_t  urc_retval = 0;
  uint16_t parsedlen = 0;
  int32_t cmdret;
  char *align_ptr;

  /* force to use google DNS service : "8.8.8.8" */
  snprintf(CmdString, 36,"AT+QIDNSCFG=%d,\"%s\"\r\n", ContextID, "192.168.111.2");
  cmdret =  AT_ExecuteCommand(Obj, UG96_TOUT_SHORT, (uint8_t *)CmdString, RET_OK | RET_ERROR);

  /* inquire the DNS service */
  if (cmdret == RET_OK)
  {
    snprintf(CmdString, 255,"AT+QIDNSGIP=%d,\"%s\"\r\n", ContextID, IPaddr_string);
    cmdret =  AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *)CmdString, RET_OK | RET_ERROR);
  }

  if (cmdret == RET_OK)
  {
    /* Receiving response from the network can take several seconds (up to 60s) */
    urc_retval = AT_RetrieveUrc(Obj, UG96_TOUT_60000, &parsedlen, UG96_BUFFER_MODE );
    if (urc_retval == RET_URC_DNS)
    {
      align_ptr = (char *) Obj->CmdResp+2;
      ParseIP(align_ptr, IPaddr_int);
      ret = UG96_RETURN_OK;
    }
  }
#ifdef UG96_DBG
  char error_string[40];
  if (ret == UG96_RETURN_OK)
  {
    configPRINTF(("Host addr from DNS: %d.%d.%d.%d\r\n", IPaddr_int[0],IPaddr_int[1],IPaddr_int[2],IPaddr_int[3]));
  }
  else
  {
    UG96_RetrieveLastErrorDetails(Obj, error_string);
    configPRINTF(("UG96_DNS_LookUp error: %s\r\n", error_string));
  }
#endif
  return ret;
}

/**
  * @brief  Configure and Start a Client connection.
  * @param  Obj: pointer to module handle
  * @param  ContextID : range is 1-20 (max three are connected simultaneously)
  * @param  conn: pointer to the connection structure
  * @retval Operation Status.
  */
UG96_Return_t  UG96_OpenClientConnection(Ug96Object_t *Obj, uint8_t ContextID, UG96_Conn_t *conn)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;
  char  type_string[13];
  int16_t recv;
  int32_t cmdret, retr_errcode;

  Obj->fops.IO_FlushBuffer();
  Obj->SocketInfo[conn->ConnectID].Type = conn->Type;
  Obj->SocketInfo[conn->ConnectID].AccessMode = conn->AccessMode;
  Obj->SocketInfo[conn->ConnectID].ComulatedQirdData = 0;
  Obj->SocketInfo[conn->ConnectID].HaveReadLength = 0;
  Obj->SocketInfo[conn->ConnectID].UnreadLength = 0;

  if (conn->Type == UG96_TCP_CONNECTION)
  {
    strcpy(type_string, "TCP");
  }
  else if (conn->Type == UG96_UDP_CONNECTION)
  {
    strcpy(type_string, "UDP");
  }
  else if (conn->Type == UG96_TCP_LISTENER_CONNECTION)
  {
    strcpy(type_string, "TCP_LISTENER");
  }
  else if (conn->Type == UG96_UDP_SERVER_CONNECTION)
  {
    strcpy(type_string, "UDP_SERVER");
  }

#ifdef UG96_DBG
        configPRINTF(("UG96_OpenClientConnection()\r\n"));
#endif
  snprintf(CmdString, UG96_CMD_SIZE, "AT+QIOPEN=%d,%d,\"%s\",\"%s\",%u,%u,%d\r\n", ContextID, conn->ConnectID, type_string, conn->Url, conn->RemotePort, conn->LocalPort, conn->AccessMode);
  /* The maximum timeout of TCP connect is 75 seconds, hence UG96_TOUT_150000/2 */
  cmdret = AT_ExecuteCommand(Obj, UG96_TOUT_150000/2, (uint8_t *)CmdString, RET_OPEN | RET_ERROR);
  if (cmdret == RET_OPEN)
  {
    recv = AT_RetrieveData(Obj, Obj->CmdResp, 6, RET_NONE, UG96_TOUT_SHORT);
    if (recv > 0)
    {
      retr_errcode = ParseNumber((char *) Obj->CmdResp+3, NULL);
      if (  retr_errcode != 0) /* read the remaining 2 characters if error nr 3 chiphers*/
      {
         recv = AT_RetrieveData(Obj, Obj->CmdResp, 2, RET_NONE, UG96_TOUT_SHORT);
#ifdef UG96_DBG
         configPRINTF(("UG96_OpenClientConnection() retr_errcode: %ld\r\n", retr_errcode));
#endif
      }
      else
      {
#ifdef UG96_DBG
        snprintf(CmdString, UG96_CMD_SIZE, "AT+QISTATE=1,%d\r\n", conn->ConnectID);
        cmdret = AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *)CmdString, RET_OK | RET_ERROR);
#endif
        ret = UG96_RETURN_OK;
      }
    }
    else
    {
#ifdef UG96_DBG
        configPRINTF(("UG96_OpenClientConnection() missing part of the response\r\n"));
#endif
    }
  }

  return ret;
}

/**
  * @brief  Stop Client connection.
  * @param  Obj: pointer to module handle
  * @param  conn: pointer to the connection structure
  * @retval Operation Status.
  */
UG96_Return_t  UG96_CloseClientConnection(Ug96Object_t *Obj, UG96_Conn_t *conn)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;
  uint32_t tickstart = Obj->GetTickCb();
  uint32_t unackedbytes = 1; /* Unknown by default, consider not NULL */
  Obj->fops.IO_FlushBuffer();

   /* Make the modem TX buffer is emtpy, i.e. sent packets have been acked
      or close socket anyway after 2 seconds delay */
  while ( (TimeLeftFromExpiration(tickstart, Obj->GetTickCb(), UG96_TOUT_5000) > 0)
         && (unackedbytes != 0))
  {
    /* Update SocketInfo for next iteration*/
    snprintf(CmdString,24,"AT+QISEND=%d,0\r\n", conn->ConnectID);
    AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *) CmdString, RET_ERROR | RET_SEND);
    AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_CRLF, UG96_TOUT_300);
    ParseQISEND((char *) Obj->CmdResp, &unackedbytes);
    /* OK */
    AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_OK, UG96_TOUT_SHORT);
#ifdef UG96_DBG
    //configPRINTF(("Unack'ed data in modem: %d bytes\r\n", unackedbytes));
#endif
  }

  snprintf(CmdString, 24, "AT+QICLOSE=%d,%d\r\n", conn->ConnectID, 15);
  ret = (UG96_Return_t) AT_ExecuteCommand(Obj, UG96_TOUT_5000, (uint8_t *)CmdString, RET_OK | RET_ERROR);
#ifdef UG96_DBG
  configPRINTF(("UG96_CloseClientConnection(), ret value: %d\r\n", ret));
#endif
  return ret;
}


/**
  * @brief  Send an amount data over C2C.
  * @param  Obj: pointer to module handle
  * @param  Socket: number of the socket
  * @param  pdata: pointer to data
  * @param  Reqlen : (IN) nr of bytes to be sent
  * @param  SentLen : (OUT) ptr to return the nr of bytes actually sent
  * @param  Timeout : time (ms) for the modem to confirm the data was sent. Function can take twice to return.
  * @retval Operation Status.
  */
UG96_SendRet_t  UG96_SendData(Ug96Object_t *Obj, uint8_t Socket, uint8_t *pdata, uint16_t Reqlen , uint16_t *SentLen , uint32_t Timeout)
{
  UG96_SendRet_t ret = UG96_SEND_RET_CONN_ERR;

  *SentLen = 0;
  if(Reqlen <= UG96_TX_DATABUF_SIZE )
  {
    snprintf(CmdString, 24, "AT+QISEND=%d,%d\r\n", Socket, Reqlen);
    AT_ExecuteCommand(Obj, Timeout,  (uint8_t *) CmdString, RET_ERROR | RET_ARROW);
    ret = AT_RequestSendData(Obj, pdata, Reqlen, Timeout);
    if(ret == RET_SENT)
    {
	  *SentLen =  Reqlen;
    }
  }
#ifdef UG96_DBG
  if(ret == RET_SENT)
  {
    configPRINTF(("UG96_SendData() OK sending from STM32 to UG96: length: %u\r\n", Reqlen));
  }
  else
  {
    configPRINTF(("UG96_SendData() FAIL sending: length: %u, ret error: %d\r\n", Reqlen, ret));
  }
#endif
  return ret;
}


/**
  * @brief  Receive an amount data over C2C.
  * @param  Obj: pointer to module handle
  * @param  Socket: number of the socket
  * @param  pdata: pointer to data
  * @param  Requestedlen (IN) : in UG96_BUFFER_MODE the req len, in UG96_DIRECT_PUSH is the max leng available in pdata[] buffer
  * @param  Receivedlen (OUT) : pointer to return the length of the data received
  * @param  Timeout : timeout (ms) used by each internal exchange Mcu<-->modem; hence function could take much longer to return
  * @retval Operation Status.
  */
UG96_ReceiveRet_t  UG96_ReceiveData(Ug96Object_t *Obj, uint8_t Socket, uint8_t *pdata, uint16_t Requestedlen, uint16_t *Receivedlen, uint32_t Timeout)
{

  UG96_ReceiveRet_t ret = UG96_RECEIVE_RET_COM_ERR;
  int32_t  urc_retval = 0, rcvlen = 0;
  uint8_t *ptr = pdata;
  uint16_t parsedlen = 0;
  uint8_t parse_count;
  int16_t  qird_retval = 0;

  *Receivedlen = 0;

  if (Obj->SocketInfo[Socket].Type != UG96_TCP_CONNECTION)
  {
      return UG96_RECEIVE_RET_PARAM_ERR; /* currently only TCP connection is implemented */
  }

  if(Requestedlen <= UG96_RX_DATABUF_SIZE )
  {
    switch (Obj->SocketInfo[Socket].AccessMode) {

      case UG96_BUFFER_MODE:
#ifdef UG96_DBG
        configPRINTF(("UG96_ReceiveData() Requestedlen: %d\r\n", Requestedlen));
        configPRINTF(("UG96_ReceiveData() UnreadLength: %d\r\n", Obj->SocketInfo[Socket].UnreadLength));
        configPRINTF(("UG96_ReceiveData() ComulatedQirdData: %d\r\n", Obj->SocketInfo[Socket].ComulatedQirdData));
#endif

        if(Obj->SocketInfo[Socket].UartRemaining > 0)  /* if Timeout respects UART speed this should never happen*/
        {
#ifdef UG96_DBG
            configPRINTF(("UG96_ReceiveData() UartRemaining %d\r\n", Obj->SocketInfo[Socket].UartRemaining));
#endif
            rcvlen = AT_RetrieveData(Obj, ptr, Obj->SocketInfo[Socket].UartRemaining, RET_NONE, Timeout);
            Obj->SocketInfo[Socket].UartRemaining -= rcvlen;
            *Receivedlen = rcvlen;
            if (Obj->SocketInfo[Socket].UartRemaining == 0)
            {
              /* update Obj->SocketInfo[Socket] and exit */
              snprintf(CmdString,24,"AT+QIRD=%d,0\r\n", Socket);
              AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *) CmdString, RET_ERROR | RET_READ);
              AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_CRLF, UG96_TOUT_300);
              ParseQIRD((char *) Obj->CmdResp, &Obj->SocketInfo[Socket].ComulatedQirdData);
              ret = UG96_RECEIVE_RET_OK;
            }
            else
            {
              ret = UG96_RECEIVE_RET_INCOMPLETE; /* even this iteration was not sufficient to get all data from modem */
            }
            break;
        }

        if(Obj->SocketInfo[Socket].UnreadLength > 0)          /* some data remaining from previous operation */
        {
          if(Requestedlen > Obj->SocketInfo[Socket].UnreadLength)
          {
            Requestedlen = Obj->SocketInfo[Socket].UnreadLength;    /* Just take the one to end remaining data */
          }
        }
        else
        {
          /* Wait for parsing URC */
          urc_retval = AT_RetrieveUrc(Obj, Timeout, &parsedlen, UG96_BUFFER_MODE );
#ifdef UG96_DBG
          if (urc_retval < 0)
          {
            configPRINTF(("UG96_ReceiveData() URC not received\r\n"));
          }
          else
          {
            configPRINTF(("UG96_ReceiveData() URC OK\r\n"));
          }
#endif
        }

        snprintf(CmdString,24,"AT+QIRD=%d,%u\r\n", Socket, Requestedlen);
        qird_retval = AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *) CmdString, RET_ERROR | RET_READ);
        if ((qird_retval < 0) || (qird_retval == RET_ERROR))
        {
#ifdef UG96_DBG
          configPRINTF(("UG96_ReceiveData() QIRD issue\r\n"));
#endif
          return UG96_RECEIVE_RET_INCOMPLETE;
        }
        /* length parsing */
        AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_CRLF, UG96_TOUT_300);
        parsedlen = (uint16_t) ParseNumber((char *) Obj->CmdResp, &parse_count);
#ifdef UG96_DBG
        if ((Obj->SocketInfo[Socket].UnreadLength > 0) && (parsedlen != Requestedlen))
        {
          configPRINTF(("UG96_ReceiveData() unexpected behaviour parsedlen != Requestedlen (%u)\r\n", Requestedlen));
        }
        configPRINTF(("UG96_ReceiveData() parsedlen is %u\r\n", parsedlen));
#endif
        /* Retrieving data */
        rcvlen = AT_RetrieveData(Obj, ptr, parsedlen, RET_NONE, Timeout);
        if (rcvlen < 0)
        {
          *Receivedlen = 0;
           break; /* return UG96_RECEIVE_RET_COM_ERR */
        }
        if(rcvlen != parsedlen) /* uart has not retrieved all data from modem yet*/
        {
#ifdef UG96_DBG
          configPRINTF(("UG96_ReceiveData() Received length mismatch!!!\r\n"));
#endif
          Obj->SocketInfo[Socket].UartRemaining = parsedlen - rcvlen;
          Obj->SocketInfo[Socket].UnreadLength -= rcvlen;
          *Receivedlen = rcvlen;
          ret =  UG96_RECEIVE_RET_INCOMPLETE; /* if Timeout respects UART speed this should never happen*/
        }
        else
        {
#ifdef UG96_DBG
          configPRINTF(("UG96_ReceiveData() Received  OK %ld\r\n", rcvlen));
#endif
          *Receivedlen = rcvlen;
          Obj->SocketInfo[Socket].UartRemaining = 0;

          /* OK */
          AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_OK, UG96_TOUT_SHORT);

          /* Update SocketInfo for next iteration*/
          snprintf(CmdString,24,"AT+QIRD=%d,0\r\n", Socket);
          AT_ExecuteCommand(Obj, UG96_TOUT_300, (uint8_t *) CmdString, RET_ERROR | RET_READ);
          AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_CRLF, UG96_TOUT_300);
          ParseQIRD((char *) Obj->CmdResp, &Obj->SocketInfo[Socket].ComulatedQirdData);

          /* OK */
          AT_RetrieveData(Obj, Obj->CmdResp, 0, RET_OK, UG96_TOUT_SHORT);

          ret =  UG96_RECEIVE_RET_OK;
        }

        break;

    case UG96_DIRECT_PUSH:   /* ******** Careful: NOT fully TESTED ******** */

        if(Obj->RemainRxData > 0)   /* some data remaining from previous operation */
        {
          if(Requestedlen <= Obj->RemainRxData)       /* in UG96_DIRECT_PUSH Requestedlen is the max the applic can receive */
          {
            Obj->RemainRxData -= Requestedlen;
            if(AT_RetrieveData(Obj, ptr, Requestedlen, RET_NONE, Timeout) != Requestedlen)
            {
              return UG96_RECEIVE_RET_COM_ERR;
            }
            else
            {
              return UG96_RECEIVE_RET_OK;
            }
          }
          else                              /* all remaining data can be retried */
          {
            Requestedlen -= Obj->RemainRxData;

            if( AT_RetrieveData(Obj, ptr, Obj->RemainRxData, RET_NONE, Timeout) == Obj->RemainRxData)
            {
              ptr += Obj->RemainRxData;
              Obj->RemainRxData = 0;
            }
            else
            {
              /* error */
              return UG96_RECEIVE_RET_COM_ERR;
            }
          }
        }

        urc_retval = AT_RetrieveUrc(Obj, Timeout, &parsedlen, UG96_DIRECT_PUSH );

        if( urc_retval == RET_URC_RECV)
        {
          if(parsedlen > 0)
          {
            if(Requestedlen > parsedlen)    /* retrieve all data */
            {
              Obj->RemainRxData = 0;
              Requestedlen = parsedlen;
            }
            else                      /* not enough space in the applic buffer */
            {
              Obj->RemainRxData = parsedlen - Requestedlen;
            }

            *Receivedlen = AT_RetrieveData(Obj, ptr, Requestedlen, RET_NONE, Timeout);
            if(*Receivedlen == Requestedlen)
            {
              ret = UG96_RECEIVE_RET_OK;
            }
          }
        }
        break;

      case UG96_TRANSPARENT_MODE:
        /* TBD */
        ret = UG96_RECEIVE_RET_PARAM_ERR;
        break;

    } /*end switch case */
  }
  else
  {
    ret = UG96_RECEIVE_RET_PARAM_ERR;
  }
  return ret;
}

/* Eseye */
UG96_Return_t  UG96_GetCrsmData(Ug96Object_t *Obj, int ef, unsigned int offset, unsigned int length)
{
  UG96_Return_t ret = UG96_RETURN_ERROR;

  snprintf(CmdString, UG96_CMD_SIZE,"AT+CRSM=%d,%d,%d,%d,%d,,\"3F007FEE\"\r\n", 176, ef, (offset & 0xff00) >> 8, offset & 0xff, length);
  ret = (UG96_Return_t) AT_ExecuteCommand(Obj, UG96_TOUT_15000, (uint8_t *)CmdString, RET_OK | RET_ERROR | RET_CME_ERROR );

  return ret;
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
