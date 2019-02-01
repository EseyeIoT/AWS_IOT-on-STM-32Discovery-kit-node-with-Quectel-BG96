#ifndef ESEYE_HEADER_INCLUDED
#define ESEYE_HEADER_INCLUDED

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "c2c.h"

#include "stm32l475e_iot01_hsensor.h"
#include "stm32l475e_iot01_psensor.h"
#include "stm32l475e_iot01_tsensor.h"

#define NETWORK_SCAN_RUN	0	// Change to 1 to enable

#if NETWORK_SCAN_RUN
#define MAJOR_VERSION 99
#define MINOR_VERSION 1
#else
#define MAJOR_VERSION 1
#define MINOR_VERSION 7
#endif

enum SIM_FILES_STATE {
	AN_SIM_INCOMPLETE = 0,
	AN_SIM_COMPLETE
};

enum CONNECTION_STATE {
	AN_CONNECTING = 0,
	AN_CONNECTED,
	AN_DISCONNECTED
};

enum ANYNET_FILE_TYPES{
	AN_THINGNAME = 0,
	AN_URLFULL = 1,
	AN_ROOTCA = 2,
	AN_CLIENTCERT = 3,
	AN_PRIVATEKEY = 4
};

struct anynet_file_details {
	enum ANYNET_FILE_TYPES type;
	int8_t *data;
	uint16_t ef;
	uint16_t df;
	uint8_t base64;
	uint16_t data_length;
	uint32_t crc;

};

extern volatile uint8_t use_cellular_socket;
extern SemaphoreHandle_t xCellularSemaphoreHandle;
extern volatile enum CONNECTION_STATE conn_led;
extern volatile enum SIM_FILES_STATE got_certs;

int C2C_HwStatus(void);
void C2C_HwStatusInit(void);
void C2C_HwPowerDown(void);
int cellular_init(void);
C2C_SendStatus_t C2C_SendData(uint32_t socket, uint8_t *pdata, uint16_t Reqlen, uint16_t *SentDatalen, uint32_t Timeout);
C2C_Ret_t C2C_ReceiveData(uint32_t socket, uint8_t *pdata, uint16_t Reqlen, uint16_t *RcvDatalen, uint32_t Timeout);
C2C_Ret_t C2C_StopClientConnection(uint32_t socket);
#endif
