#include "eseye.h"

#ifdef USE_ESEYE
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

#include "ug96.h"
#include "ug96_io.h"
#include "c2c.h"

#include <stdbool.h>
#include <stdlib.h>

#define C2C_CONTEXT_1   1
#define  CELL_CONNECT_MAX_ATTEMPT_COUNT  6

#define ANYNET_FILE_HDRLEN 6
#define ANYNET_FILE_MAXLEN 2048
#define ANYNET_MAX_CHUNK 64
static int anynet_collate_nibbles(int nibbleswap, uint8_t *inbuf, uint8_t *outbuf, int numnibbles);
int anynet_decode_response(uint8_t *converted, uint16_t length);
static char* anynet_base64_encode(char *data, unsigned int input_length, unsigned int *output_length);
int anynet_convert_certs(void);
static int anynet_collate_nibbles(int nibbleswap, uint8_t *inbuf, uint8_t *outbuf, int numnibbles);
int anynet_decode_response(uint8_t *converted, uint16_t length);
int anynet_get_headers(void);
int anynet_get_data(void);
int anynet_get_sim_data(uint32_t idx, uint8_t* data, uint16_t data_length, uint16_t *bytes_read);
int anynet_convert_certs(void);

const char base64_encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', '+', '/'};

const uint32_t crc32_table[] =
{
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};


static int mod_table[] = {0, 2, 1};
static char* anynet_base64_encode(char *data, unsigned int input_length, unsigned int *output_length);

static void C2C_ConvertIpAddrToString(const uint8_t* IpArray, char* ReturnString);
int  C2C_StartClientConnection(uint32_t socket, int type, const char* host_url, uint8_t* ipaddr, uint16_t port, uint16_t local_port);

volatile enum CONNECTION_STATE conn_led = AN_CONNECTING;
volatile enum SIM_FILES_STATE got_certs = AN_SIM_INCOMPLETE;
volatile uint8_t use_cellular_socket = 1;
uint8_t anynet_temp[2048];
static StaticSemaphore_t xCellularSemaphoreBuffer;
SemaphoreHandle_t xCellularSemaphoreHandle;

/* Storage for anynet secure parameters */
int8_t anynet_thingname[256] = {0};
int8_t anynet_urlfull[256] = {0};
int8_t anynet_rootca[2048] = {0};
int8_t anynet_publiccert[2048] = {0};
int8_t anynet_privatekey[2048] = {0};
uint8_t anynet_converted[256];


struct anynet_file_details anynet_sim_file_data[5] = {
		{AN_THINGNAME,  anynet_thingname,  0x6FE0, 0x7FEE, 0, 0, 0},
		{AN_URLFULL,    anynet_urlfull,    0x6FE4, 0x7FEE, 0, 0, 0},
		{AN_ROOTCA,     anynet_rootca,     0x6FE1, 0x7FEE, 1, 0, 0},
		{AN_CLIENTCERT, anynet_publiccert, 0x6FE2, 0x7FEE, 1, 0, 0},
		{AN_PRIVATEKEY, anynet_privatekey, 0x6FE3, 0x7FEE, 1, 0, 0},
};

static const char ap_code[] = "eseye1";
static const char username[] = "reinvent";
static const char password[] = "demo";

static void C2C_HwResetAndPowerUp(void);
static C2C_Ret_t C2C_ConfigureAP(uint8_t ContextType,
		const char* Apn,
		const char* Username,
		const char* Password,
		uint8_t AuthenticationMode);
static C2C_Ret_t C2C_Connect(void);


Ug96Object_t Ug96C2cObj;
static uint16_t rnglocalport = 0;

/***********************************************************************/

uint32_t InitSensors(void)
{
	if(BSP_PSENSOR_Init() != PSENSOR_OK) return 0;
	if(BSP_HSENSOR_Init() != HSENSOR_OK) return 0;
	if(BSP_TSENSOR_Init() != TSENSOR_OK) return 0;
	//	if(BSP_ACCELERO_Init() != ACCELERO_OK) return 0;
	//	if(BSP_GYRO_Init() != GYRO_OK) return 0;
	//	if(BSP_MAGNETO_Init() != MAGNETO_ERROR) return 0;

	// success
	return 1;
}

int cellular_init(void)
{
	C2C_Ret_t ret = C2C_RET_ERROR;
	C2C_RegiStatus_t reg_status = C2C_REGISTATUS_UNKNOWN;
	uint8_t c2cConnectCounter = 0;

#if ONLY_2G
	configPRINTF(("*** Firmware for 2G only ***\r\n"));
#endif
	configPRINTF(("*** Eseye Anynet Secure connection ***\r\n"));

	if(InitSensors() != 1)
	{
		configPRINTF(("Error in InitSensors\r\n"));
	}

	while (ret != C2C_RET_OK) {
		/*  C2C Module power up and initialization */
		configPRINTF(("Power Up the C2C module\r\n"));
		C2C_HwResetAndPowerUp();
		/* wait additional 2 sec: even the modem is ON, registering can fail if request is done too early */
		vTaskDelay(pdMS_TO_TICKS(2000));

		reg_status =  C2C_Init(300);
		switch (reg_status) {
		case C2C_REGISTATUS_HOME_NETWORK:
		case C2C_REGISTATUS_ROAMING:
#ifdef USE_BG96
		case C2C_REGISTATUS_UNKNOWN:
#endif
			//			BSP_LED_On(LED_GREEN);
			ret = C2C_RET_OK;
			configPRINTF(("C2C module registered\r\n"));
			break;
		case C2C_REGISTATUS_TRYING:
			configPRINTF(("C2C registration trying\r\n"));
			break;
		case C2C_REGISTATUS_REG_DENIED:
			configPRINTF(("C2C registration denied\r\n"));
			break;
		case C2C_REGISTATUS_NOT_REGISTERED:
			configPRINTF(("C2C registration failed\r\n"));
			break;
		case C2C_REGISTATUS_ERROR:
			configPRINTF(("C2C AT comunication error with the C2C device\r\n"));
			configPRINTF(("C2C device might be disconnected or wrongly connected\r\n"));
			break;
		case C2C_REGISTATUS_SIM_NOT_INSERTED:
			configPRINTF(("SIM is not inserted\r\n"));
			break;
		default:
			configPRINTF(("C2C SIM error: %d\r\n", reg_status));
			configPRINTF(("Please check if SIM is inserted & valid, if credentials are ok, etc.\r\n"));
			break;
		}

		if(ret != C2C_RET_OK)
		{
			/*  we've already tried both or ret is OK, so let's quit */
			break;
		}
	}

	if (ret == C2C_RET_OK)
	{
		/* Connect to the specified APN. */
		ret = C2C_ConfigureAP(1, ap_code, username, password, UG96_AUTHENT_CHAP);
		configPRINTF(("\rConnecting to AP: be patient ...\n"));

		do
		{
			ret = C2C_Connect();
			c2cConnectCounter++;
			if (ret == C2C_RET_OK)
			{
				break;
			}
			else
			{
				configPRINTF(("Connection try %d failed\r\n", c2cConnectCounter));
				vTaskDelay(pdMS_TO_TICKS(10000));
			}
		}
		while (1);//c2cConnectCounter < CELL_CONNECT_MAX_ATTEMPT_COUNT);

		//BSP_LED_Off(LED_GREEN);
		if (ret == C2C_RET_OK)
		{
			configPRINTF(("Connected to AP\r\n"));
		}
		else
		{
			configPRINTF(("Failed to connect to AP\r\n"));
		}
	}

	/* create semaphore to serialise access to modem */
	xCellularSemaphoreHandle = xSemaphoreCreateMutexStatic( &( xCellularSemaphoreBuffer ) );

	/* Initialize semaphore. */
	xSemaphoreGive( xCellularSemaphoreHandle );

	if(ret != C2C_RET_OK)
	{
		C2C_HwPowerDown();
		return -1;
	}

	return 0;
}

void C2C_HwStatusInit(void)
{
	GPIO_InitTypeDef  GPIO_InitStructPwr, GPIO_InitStructStatus;

//	C2C_RST_GPIO_CLK_ENABLE();
	C2C_PWRKEY_GPIO_CLK_ENABLE();
	C2C_STATUS_GPIO_CLK_ENABLE();

	/* STATUS */
	GPIO_InitStructStatus.Pin       = C2C_STATUS_PIN;
	GPIO_InitStructStatus.Mode      = GPIO_MODE_INPUT;
	GPIO_InitStructStatus.Pull      = GPIO_PULLDOWN;
	GPIO_InitStructStatus.Speed     = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(C2C_STATUS_GPIO_PORT, &GPIO_InitStructStatus);

	/* PWRKEY */
	GPIO_InitStructPwr.Pin       = C2C_PWRKEY_PIN;
	GPIO_InitStructPwr.Mode      = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructPwr.Pull      = GPIO_NOPULL;
	GPIO_InitStructPwr.Speed     = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(C2C_PWRKEY_GPIO_PORT,&GPIO_InitStructPwr);
	HAL_GPIO_WritePin(C2C_PWRKEY_GPIO_PORT, C2C_PWRKEY_PIN,GPIO_PIN_RESET);
	vTaskDelay(pdMS_TO_TICKS(1000));

	if(HAL_GPIO_ReadPin(C2C_STATUS_GPIO_PORT, C2C_STATUS_PIN) == 1)
	{
		C2C_HwPowerDown();
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

int C2C_HwStatus(void)
{
	return HAL_GPIO_ReadPin(C2C_STATUS_GPIO_PORT, C2C_STATUS_PIN);
}

void C2C_HwPowerDown(void)
{
	HAL_GPIO_WritePin(C2C_PWRKEY_GPIO_PORT, C2C_PWRKEY_PIN,GPIO_PIN_SET);
	vTaskDelay(pdMS_TO_TICKS(2000));

	HAL_GPIO_WritePin(C2C_PWRKEY_GPIO_PORT, C2C_PWRKEY_PIN,GPIO_PIN_RESET);
	vTaskDelay(pdMS_TO_TICKS(2000));
	conn_led = AN_DISCONNECTED;
	got_certs = AN_SIM_INCOMPLETE;
}

static void C2C_HwResetAndPowerUp(void)
{

#ifndef USE_BG96
	// default case is UG96 modem
	HAL_GPIO_WritePin(C2C_RST_GPIO_PORT, C2C_RST_PIN, GPIO_PIN_SET);
	HAL_Delay(200);
	HAL_GPIO_WritePin(C2C_RST_GPIO_PORT, C2C_RST_PIN, GPIO_PIN_RESET);
	HAL_Delay(150);

	HAL_GPIO_WritePin(C2C_PWRKEY_GPIO_PORT, C2C_PWRKEY_PIN, GPIO_PIN_SET);
	HAL_Delay(150);
	HAL_GPIO_WritePin(C2C_PWRKEY_GPIO_PORT, C2C_PWRKEY_PIN, GPIO_PIN_RESET);
	/* Waits for Modem complete its booting procedure */
	HAL_Delay(2300);
#else /* USE_BG96 */
	// BG96 case
	/* POWER DOWN */
	//  HAL_GPIO_WritePin(C2C_PWRKEY_GPIO_PORT, C2C_PWRKEY_PIN, GPIO_PIN_SET);
	//  HAL_GPIO_WritePin(C2C_RST_GPIO_PORT, C2C_RST_PIN, GPIO_PIN_SET);
	//  vTaskDelay(pdMS_TO_TICKS(150));
	//
	//  /* POWER UP */
	//  HAL_GPIO_WritePin(C2C_PWRKEY_GPIO_PORT, C2C_PWRKEY_PIN, GPIO_PIN_RESET);
	//  HAL_GPIO_WritePin(C2C_RST_GPIO_PORT, C2C_RST_PIN, GPIO_PIN_RESET);
	//  vTaskDelay(pdMS_TO_TICKS(100));
	//

	HAL_GPIO_WritePin(C2C_PWRKEY_GPIO_PORT, C2C_PWRKEY_PIN,GPIO_PIN_SET);
	vTaskDelay(pdMS_TO_TICKS(600));

	HAL_GPIO_WritePin(C2C_PWRKEY_GPIO_PORT, C2C_PWRKEY_PIN,GPIO_PIN_RESET);
	/* Waits for Modem complete its booting procedure */
	vTaskDelay(pdMS_TO_TICKS(5000));

	conn_led = AN_CONNECTING;
#endif /* USE_BG96 */
}

/**
 * @brief  Configure a TCP/UDP Access Point
 * @param  ContextType : 1: IPV4
 * @param  Apn : access point name
 * @param  Username : Got IP Address
 * @param  Password : Network IP mask
 * @param  AuthenticationMode : 0: none, 1: PAP
 * @retval Operation status
 */
static C2C_Ret_t C2C_ConfigureAP(uint8_t ContextType,
		const char* Apn,
		const char* Username,
		const char* Password,
		uint8_t AuthenticationMode)
{
	C2C_Ret_t ret = C2C_RET_ERROR;
	UG96_APConfig_t ApConfig;

	ApConfig.ContextID = C2C_CONTEXT_1;
	strncpy((char*)ApConfig.ApnString, (char*)Apn, UG96_MAX_APN_NAME_SIZE);
	strncpy((char*)ApConfig.Username, (char*)Username, UG96_MAX_USER_NAME_SIZE);
	strncpy((char*)ApConfig.Password, (char*)Password, UG96_MAX_PSW_NAME_SIZE);
	ApConfig.Authentication = (UG96_Authent_t) AuthenticationMode;


	if(UG96_ConfigureAP(&Ug96C2cObj, &ApConfig) == UG96_RETURN_OK)
	{
		ret = C2C_RET_OK;
	}
	return ret;
}

/**
 * @brief  Join a PDP Access Point
 * @retval Operation status
 */
static C2C_Ret_t C2C_Connect(void)
{
	C2C_Ret_t ret = C2C_RET_ERROR;

	if(UG96_Activate(&Ug96C2cObj, C2C_CONTEXT_1) == UG96_RETURN_OK)
	{
		ret = C2C_RET_OK;
	}
	return ret;
}

C2C_RegiStatus_t C2C_Init(uint16_t registration_timeout_sec)
{
	UG96_InitRet_t  init_status;
	UG96_SIMState_t sim_tmp;
	C2C_RegiStatus_t ret = C2C_REGISTATUS_ERROR;
	bool signalMessage = false;

	/* Channel signal quality */
	int32_t quality_level = 0;
	int8_t quality_level_db = 0;

	/* counter for PS attachment */
	int8_t attRetry = 0;

	/* Init for timeout management*/
	uint32_t tickstart;
	uint32_t tickcurrent;
	uint32_t registration_timeout_msec = registration_timeout_sec*1000;

	memset(Ug96C2cObj.Imei, 0, sizeof(Ug96C2cObj.Imei));

	tickstart = xTaskGetTickCount();
	UG96_RegisterTickCb(&Ug96C2cObj, xTaskGetTickCount);

	if(UG96_RegisterBusIO(&Ug96C2cObj,
			UART_C2C_Init,
			UART_C2C_DeInit,
			UART_C2C_SetBaudrate,
			UART_C2C_SendData,
			UART_C2C_ReceiveSingleData,
			UART_C2C_FlushBuffer) == UG96_RETURN_OK)
	{

		init_status =  UG96_Init(&Ug96C2cObj);

		if(init_status == UG96_INIT_RET_OK)
		{
#if NETWORK_SCAN_RUN
/* This will search the available networks and print on console */
			/* Wait 2s before running this command */
			vTaskDelay(pdMS_TO_TICKS(2000));
			while (1) {
				if (UG96_RETURN_OK != UG96_NetworkSearch(&Ug96C2cObj)) {
					configPRINTF(("Network scan failed, waiting 60s then running again\r\n"));
				} else {
					configPRINTF(("Scan Successful, you can unplug or wait 60s for scan repeat\r\n"));
				}
				vTaskDelay(pdMS_TO_TICKS(60000));
			}
#endif
			got_certs = AN_SIM_INCOMPLETE;

/* PN add CS registration check first */
			tickcurrent = xTaskGetTickCount() - tickstart;
			while((tickcurrent <  registration_timeout_msec) || (registration_timeout_sec == C2C_MAX_DELAY))
			{
				/* Check Circuit Switched Registration */
				configPRINTF(("Attempting to register on circuit switched mobile network\r\n"));
				ret = (C2C_RegiStatus_t) UG96_GetCsNetworkRegistrationStatus(&Ug96C2cObj);
				if ((ret == C2C_REGISTATUS_HOME_NETWORK) || (ret == C2C_REGISTATUS_ROAMING))
				{
					tickcurrent = xTaskGetTickCount() - tickstart;
					configPRINTF(("Registration done in %lu milliseconds\r\n", tickcurrent));
					/* registered step 1 now bail out */
					break;
				}
				else
				{
					/* PN wait 5s here */
					vTaskDelay(pdMS_TO_TICKS(5000));
				}
				tickcurrent = xTaskGetTickCount() - tickstart;
			}

/* PN end */
			tickcurrent = xTaskGetTickCount() - tickstart;
			while((tickcurrent <  registration_timeout_msec) || (registration_timeout_sec == C2C_MAX_DELAY))
			{
				/* Check Packet Switched Registration */
				configPRINTF(("Attempting to register on packet switched mobile network\r\n"));
				ret = (C2C_RegiStatus_t) UG96_GetPsNetworkRegistrationStatus(&Ug96C2cObj);
				if ((ret == C2C_REGISTATUS_HOME_NETWORK) || (ret == C2C_REGISTATUS_ROAMING))
				{
					tickcurrent = xTaskGetTickCount() - tickstart;
					configPRINTF(("Registration done in %lu msseconds\r\n", tickcurrent));
					/* check signal */
					if(UG96_GetSignalQualityStatus(&Ug96C2cObj, &quality_level) == UG96_RETURN_OK)
					{
						if (quality_level == 99)
						{
							if (signalMessage == false)
							{
								configPRINTF(("Signal not known or not detectable yet (be patient)\r\n"));
								signalMessage = true;
							}
							else
							{
								configPRINTF(("."));
							}
							vTaskDelay(pdMS_TO_TICKS(3000));
						}
						else
						{
							uint8_t cert_tries = 0;

							quality_level_db = (int8_t) (-113 + 2*quality_level);
							configPRINTF(("\nSignal Level: %d dBm\r\n", quality_level_db));
							while(got_certs == 0 && cert_tries != 10)
							{
								configPRINTF(("Getting credentials please wait...\r\n"));
								if(anynet_get_headers() == 1)
								{
									if(anynet_get_data() == 1)
									{
										got_certs = AN_SIM_COMPLETE;
									}
								}

								if(got_certs != AN_SIM_COMPLETE)
								{
									configPRINTF(("Credentials not ready, checking again in 30 seconds\r\n"));
									vTaskDelay(pdMS_TO_TICKS(30000));
									++cert_tries;
								}
							}
							if(got_certs == AN_SIM_COMPLETE)
							{
								configPRINTF(("Credentials obtained\r\n"));
								break;
							}
							else
							{
								configPRINTF(("Credentials could not be obtained\r\n"));
								return C2C_REGISTATUS_ERROR;
							}
						}
					}
				}
				else
				{
					vTaskDelay(pdMS_TO_TICKS(3000));
				}

				tickcurrent = xTaskGetTickCount() - tickstart;
			}

			if(got_certs == AN_SIM_COMPLETE)
			{
				tickcurrent = xTaskGetTickCount() - tickstart;
				while((tickcurrent <  registration_timeout_msec) || (registration_timeout_sec == C2C_MAX_DELAY))
				{
					/* attach the MT to the packet domain service */
					if(UG96_PSAttach(&Ug96C2cObj) == UG96_RETURN_OK)
					{
						configPRINTF(("Packet Switched attachment succeeded\r\n"));
						break;
					}
					else
					{
						/* start an automatic PLMN selection */
						attRetry++;
						if (attRetry == 1)
						{
							configPRINTF(("Trying an automatic registration. It may take until 3 minutes, please wait ...\r\n"));
							if (UG96_RETURN_OK != UG96_AutomaticPlmnSelection(&Ug96C2cObj))
								break;
						}
						else
						{
							if (attRetry > 4)
							{
								configPRINTF(("Unrecoverable Error, PS attachment failed\r\n"));
								break;
							}
						}
						vTaskDelay(pdMS_TO_TICKS(1000));
					}
					tickcurrent = xTaskGetTickCount() - tickstart;
				}
			}
		}
		else
		{
			if(init_status == UG96_INIT_RET_SIM_ERR)
			{
				sim_tmp = Ug96C2cObj.SimInfo.SimStatus;
				if (sim_tmp == UG96_OPERATION_NOT_ALLOW)
				{
					ret = C2C_REGISTATUS_SIM_OP_NOT_ALLOWED;
				}
				else
				{
					ret = (C2C_RegiStatus_t) Ug96C2cObj.SimInfo.SimStatus;
				}
			}
			else
			{
				ret = C2C_REGISTATUS_ERROR; /* generic e.g. module does not respond to AT command */
			}
		}
	}
	return ret;
}

/**
 * @brief  for a given host name it enquires the DNS google service to resolve the Ip addr
 * @param  host_addr_string: domain name (e.g. www.amazon.com)
 * @param  ipaddr : pointer where to store the retrieved Ip addr in uint8_t[4] format
 * @retval Operation status
 */
C2C_Ret_t C2C_GetHostAddress(const char *host_addr_string, uint8_t *ipaddr)
{
	C2C_Ret_t ret = C2C_RET_ERROR;

	/* This blocking call may take several seconds before returning */
	if(UG96_DNS_LookUp(&Ug96C2cObj, C2C_CONTEXT_1, host_addr_string, ipaddr) == UG96_RETURN_OK)
	{
		ret = C2C_RET_OK;
	}
	return ret;
}

static void C2C_ConvertIpAddrToString(const uint8_t* IpArray, char* ReturnString)
{
	snprintf((char*)ReturnString, 16, "%d.%d.%d.%d", IpArray[0], IpArray[1], IpArray[2], IpArray[3]);
}

/**
 * @brief  Configure and start a client connection
 * @param  socket : Connection ID
 * @param  type : Connection type TCP/UDP
 * @param  host_url : name of the connection (e.g. www.amazon.com)
 * @param  ipaddr : Ip addr in array numbers uint8_t[4] (just used if host_url == NULL)
 * @param  port : Remote port
 * @param  local_port : Local port
 * @retval Operation status
 */
int C2C_StartClientConnection(uint32_t socket, int type, const char* host_url, uint8_t* ipaddr, uint16_t port, uint16_t local_port)
{
	C2C_Ret_t ret = C2C_RET_ERROR;
	char converted_ipaddr[16];
	UG96_Conn_t conn;
	int random_number = 0;

	conn.ConnectID = socket;
	conn.RemotePort = port;
#ifdef ACCESS_MODE_DIRECT_PUSH
	conn.AccessMode = UG96_DIRECT_PUSH; /* not fully tested */
#else
	conn.AccessMode = UG96_BUFFER_MODE;
#endif /* ACCESS_MODE_DIRECT_PUSH */

	if (local_port != 0)
	{
		conn.LocalPort = local_port;
	}
	else
	{
		/* The IANA range for ephemeral ports is 49152�65535. */
		/* implement automatic nr by sw because Queqtel assigns always the same initial nr */
		/* generate random local port  number between 49152 and 65535 */
		if (rnglocalport == 0)  /* just at first open since board reboot */
		{
			random_number = rand();
			rnglocalport = ((uint16_t) (random_number & 0xFFFF) >> 2) + 49152;
		}
		else /* from second time function execution, increment by one */
		{
			rnglocalport += 1;
		}

		if (rnglocalport < 49152) /* Wrap-around */
		{
			rnglocalport = 49152;
		}

		conn.LocalPort = rnglocalport;
	}

	switch (type) {
	case C2C_TCP_PROTOCOL:
		conn.Type = UG96_TCP_CONNECTION;
		break;
	case C2C_UDP_PROTOCOL:
		conn.Type = UG96_UDP_CONNECTION; /* not tested */
		break;
	default:
		ret = C2C_RET_NOT_SUPPORTED; /* use OpenServerConnection */
	}

	if (ret != C2C_RET_NOT_SUPPORTED)
	{
		if(host_url == NULL)
		{
			C2C_ConvertIpAddrToString(ipaddr, converted_ipaddr);
			conn.Url = converted_ipaddr;
		}
		else
		{
			conn.Url = (char *)host_url;
		}
		if(UG96_OpenClientConnection(&Ug96C2cObj, C2C_CONTEXT_1, &conn) == UG96_RETURN_OK)
		{
			ret = C2C_RET_OK;
		}
	}
	return ret;
}

/**
 * @brief  Configure and close a client connection
 * @param  socket : Connection ID
 * @retval Operation status
 */
C2C_Ret_t C2C_StopClientConnection(uint32_t socket)
{
	C2C_Ret_t ret = C2C_RET_ERROR;
	UG96_Conn_t conn;

	configPRINTF(("Closing the client connection...\r\n"));
	conn.ConnectID = socket;
	if(UG96_CloseClientConnection(&Ug96C2cObj, &conn) == UG96_RETURN_OK)
	{
		configPRINTF(("Client connection closed\r\n"));
		ret = C2C_RET_OK;
	}
	return ret;
}

/**
 * @brief  Send Data on a socket
 * @param  pdata : pointer to data to be sent
 * @param  IN: Reqlen : length of data to be sent
 * @param  OUT: SentDatalen : Data actually sent
 * @param  Timeout : time (ms) for the modem to confirm the data was sent. hence function could take longer to return.
 * @retval Operation status
 */
C2C_SendStatus_t C2C_SendData(uint32_t socket, uint8_t *pdata, uint16_t Reqlen, uint16_t *SentDatalen, uint32_t Timeout)
{
	C2C_SendStatus_t ret = C2C_SEND_ERROR;
	UG96_SendRet_t status;

	status = UG96_SendData(&Ug96C2cObj, socket, pdata, Reqlen, SentDatalen, Timeout);
	if( status == UG96_SEND_RET_SENT)
	{
		ret = C2C_SEND_OK;
	}
	if( status == UG96_SEND_RET_BUF_FULL)
	{
		ret = C2C_SEND_BUF_FULL;
	}

	return ret;
}

/**
 * @brief  Receive Data from a socket
 * @param  pdata : pointer to Rx buffer
 * @param  IN: Reqlen : in UG96_BUFFER_MODE the req len, in UG96_DIRECT_PUSH is the max leng available in pdata[] buffer
 * @param  OUT: *RcvDatalen :  pointer to length of data
 * @param  Timeout : timeout (ms) used by each internal exchange Mcu<-->modem; hence function could take much longer to return
 * @retval Operation status
 */
C2C_Ret_t C2C_ReceiveData(uint32_t socket, uint8_t *pdata, uint16_t Reqlen, uint16_t *RcvDatalen, uint32_t Timeout)
{
	C2C_Ret_t ret = C2C_RET_ERROR;
	UG96_ReceiveRet_t result;

	result = UG96_ReceiveData(&Ug96C2cObj, socket, pdata, Reqlen, RcvDatalen, Timeout);
	if( (result == UG96_RECEIVE_RET_OK) || (result == UG96_RECEIVE_RET_INCOMPLETE))
	{
		ret = C2C_RET_OK;
	}
	return ret;
}



/* sim file reading */

uint32_t cert_calc_sim_checksum( uint32_t crc, uint16_t key_len, uint8_t *p_key )
{
	uint8_t *p = p_key;

	crc = crc ^ ~((uint32_t)0);

	while (key_len--)
    {
		crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    }

	return crc ^ ~((uint32_t)0);
}


/* Modem response is in ASCII hex which needs converting back to binary (possibly with nibble-swap) */
/* This function assumes inbuf is at least numnibbles characters long and outbuf is at least numnibbles/2 bytes. */
static int anynet_collate_nibbles(int nibbleswap, uint8_t *inbuf, uint8_t *outbuf, int numnibbles)
{
	int firstnibble = 1;
	int nibblecount = 0;
	int outbyte = 0;
	uint8_t writebyte = 0;
	if (numnibbles % 2 != 0) {
		printf("ERROR: Odd number of nibbles\n");
		return -1;
	}
	while(nibblecount < numnibbles){
		uint8_t rdbyte = inbuf[nibblecount];
		if(rdbyte >= '0' && rdbyte <= '9')
			rdbyte -= '0';
		if(rdbyte >= 'a' && rdbyte <= 'f'){
			rdbyte -= 'a';
			rdbyte += 10;
		}
		if(rdbyte >= 'A' && rdbyte <= 'F'){
			rdbyte -= 'A';
			rdbyte += 10;
		}
		if(firstnibble == 1){
			if (nibbleswap == 1) {
				writebyte |= rdbyte & 0x0f;
			}else{
				writebyte |= (rdbyte << 4) & 0xf0;
			}
			firstnibble = 0;
		}else{
			if (nibbleswap == 1) {
				writebyte |= (rdbyte << 4) & 0xf0;
			}else{
				writebyte |= rdbyte & 0x0f;
			}
			outbuf[outbyte++] = writebyte;
			firstnibble = 1;
			writebyte = 0;
		}
		nibblecount++;
	}
	/* zero terminate */
	outbuf[outbyte] = 0;
	return 0;
}

int anynet_decode_response(uint8_t *converted, uint16_t length)
{
	uint8_t *data =  (uint8_t*)strstr((char*)(Ug96C2cObj.CmdResp), "144,0,\"");
	if(data != NULL) {
		data += 7;

		memset(converted, 0, sizeof(length));
		anynet_collate_nibbles(false, data, converted, length*2);

		return 1;
	}

	return 0;
}

int anynet_get_headers(void)
{
	int ii;

	for(ii = 0; ii < 5; ii++)
	{
		if(UG96_GetCrsmData(&Ug96C2cObj, anynet_sim_file_data[ii].ef, 0, ANYNET_FILE_HDRLEN) == UG96_RETURN_OK)
		{
			if(anynet_decode_response(anynet_converted, ANYNET_FILE_HDRLEN * 2))
			{
				anynet_sim_file_data[ii].data_length = anynet_converted[1] | anynet_converted[0] << 8;
				if(anynet_sim_file_data[ii].data_length == 65535)
				{
					// not got the certs yet
					anynet_sim_file_data[ii].data_length = 0;
					return 0;
				}
				anynet_sim_file_data[ii].crc = anynet_converted[5] | anynet_converted[4] << 8 | anynet_converted[3] << 16 | anynet_converted[2] << 24;
			}
			else
			{
				return 0;
			}
		}
	}
	return 1;
}


int anynet_get_data(void)
{
	uint16_t data_remaining  = anynet_sim_file_data[0].data_length;
	uint16_t chunk_len = ANYNET_MAX_CHUNK;
	uint16_t offset = ANYNET_FILE_HDRLEN;
	uint16_t data_read = 0;
	int ii;

	for(ii = 0; ii < 5; ii++)
	{
		if(anynet_sim_file_data[ii].data_length < ANYNET_MAX_CHUNK)
		{
			if(UG96_GetCrsmData(&Ug96C2cObj, anynet_sim_file_data[ii].ef, ANYNET_FILE_HDRLEN, anynet_sim_file_data[ii].data_length) == UG96_RETURN_OK)
			{
				if((anynet_decode_response(anynet_converted, anynet_sim_file_data[ii].data_length * 2)))
				{
					memcpy(anynet_sim_file_data[ii].data, anynet_converted, anynet_sim_file_data[ii].data_length);
				}
				else
				{
					configPRINTF(("Waiting for the Sim File System to be completed\r\n"));
					return 0;
				}
			}
			else
			{
				return 0;
			}
		}
		else
		{
			data_remaining  = anynet_sim_file_data[ii].data_length;
			chunk_len = ANYNET_MAX_CHUNK;
			offset = ANYNET_FILE_HDRLEN;
			data_read = 0;

			while(data_remaining > 0)
			{
				if(UG96_GetCrsmData(&Ug96C2cObj, anynet_sim_file_data[ii].ef, offset, chunk_len) == UG96_RETURN_OK)
				{
					if(anynet_decode_response(anynet_converted, chunk_len * 2)) {
						memcpy(&anynet_sim_file_data[ii].data[data_read], anynet_converted, chunk_len);
						data_read += chunk_len;
						offset += chunk_len;
					}
					else
					{
						return 0;
					}
				}
				else
				{
					return 0;
				}
				data_remaining -= chunk_len;
				if(data_remaining < ANYNET_MAX_CHUNK)
				{
					chunk_len = data_remaining;
				}
			}
		}

		// check crc
		if(cert_calc_sim_checksum(0, anynet_sim_file_data[ii].data_length, (uint8_t*)anynet_sim_file_data[ii].data) != anynet_sim_file_data[ii].crc)
		{
			configPRINTF(("Waiting for the Sim File System to be completed\r\n"));
			return 0;
		}
	}

	anynet_convert_certs();

	return 1;
}

int anynet_get_sim_data(uint32_t idx, uint8_t* data, uint16_t data_length, uint16_t *bytes_read)
{
	if(!bytes_read)
	{
		return 0;
	}
	if(idx < 0 || idx > 4)
	{
		return 0;
	}

	if( anynet_sim_file_data[idx].data_length > data_length)
	{
		return 0;
	}

	memcpy(data, anynet_sim_file_data[idx].data, anynet_sim_file_data[idx].data_length);
	*bytes_read = anynet_sim_file_data[idx].data_length;

	return 1;
}

int anynet_convert_certs(void)
{
	int ii;
	int offset;
	char *data_ptr;
	unsigned int out_length = 0, data_remaining = 0, read_idx = 0;

	for(ii = 0; ii < 5; ii++)
	{
		// convert to base64
		if(anynet_sim_file_data[ii].base64 == 1)
		{
			data_ptr = anynet_base64_encode( (char*)anynet_sim_file_data[ii].data, anynet_sim_file_data[ii].data_length, &out_length);
			// add header
			offset = 0;
			if(ii != 4)
			{
				sprintf( (char*)anynet_sim_file_data[ii].data, "-----BEGIN CERTIFICATE-----\n");
				offset = 28;
			}
			else
			{
				sprintf( (char*)anynet_sim_file_data[ii].data, "-----BEGIN RSA PRIVATE KEY-----\n");
				offset = 32;
			}

			// add '\n's
			data_remaining = out_length;
			read_idx = 0;
			while(data_remaining > 0)
			{
				if(data_remaining > 64)
				{
					memcpy(&anynet_sim_file_data[ii].data[offset], &data_ptr[read_idx], 64);
					offset += 64;
					read_idx += 64;
					data_remaining -= 64;
				}
				else
				{
					memcpy(&anynet_sim_file_data[ii].data[offset], &data_ptr[read_idx], data_remaining);
					offset += data_remaining;
					read_idx += data_remaining;
					data_remaining = 0;
				}
				anynet_sim_file_data[ii].data[offset] = '\n';
				offset += 1;
			}

			if(ii != 4)
			{
				sprintf( (char*)&anynet_sim_file_data[ii].data[offset], "-----END CERTIFICATE-----\n");
				offset += 28;
			}
			else
			{
				sprintf( (char*)&anynet_sim_file_data[ii].data[offset], "-----END RSA PRIVATE KEY-----\n");
				offset += 32;
			}

			anynet_sim_file_data[ii].data[offset] = '\0';
			offset += 1;
			anynet_sim_file_data[ii].data_length = offset;
			vPortFree(data_ptr);
			data_ptr = 0;
		}
	}

	return 1;
}

static char* anynet_base64_encode(char *data, unsigned int input_length, unsigned int *output_length) {

	*output_length = 4 * ((input_length + 2) / 3);

	char *encoded_data = pvPortMalloc(*output_length);
	int i, j;
	if (encoded_data == NULL)
		return NULL;

	for (i = 0, j = 0; i < input_length;) {
		unsigned int octet_a = i < input_length ? (unsigned char)data[i++] : 0;
		unsigned int octet_b = i < input_length ? (unsigned char)data[i++] : 0;
		unsigned int octet_c = i < input_length ? (unsigned char)data[i++] : 0;

		unsigned int triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		encoded_data[j++] = base64_encoding_table[(triple >> 3 * 6) & 0x3F];
		encoded_data[j++] = base64_encoding_table[(triple >> 2 * 6) & 0x3F];
		encoded_data[j++] = base64_encoding_table[(triple >> 1 * 6) & 0x3F];
		encoded_data[j++] = base64_encoding_table[(triple >> 0 * 6) & 0x3F];
	}

	for (i = 0; i < mod_table[input_length % 3]; i++)
		encoded_data[*output_length - 1 - i] = '=';

	return encoded_data;
}


#endif
