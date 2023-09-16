/* Includes ------------------------------------------------------------------*/
#include "app_sdu.h"
#include "app_version.h"

#include "usbd_cdc_if.h"
#include "datatypes.h"
/* Private typedef -----------------------------------------------------------*/
typedef StaticQueue_t osStaticMessageQDef_t;

typedef struct SDU_RxDecodeCtrlField {

	uint8_t m_pu8SduRxFrameBuf[SDU_FRAME_MAX_LEN];

	uint16_t m_u16SduFrameLength;
	uint16_t m_u16SduRxIndicator;


	V_FUCTION_V_SYMBOL m_xSduDecodeFrameTask;//SDU portocol decode fsm dispatch.
	Timer_t m_xSduTicksForRxFrameTimeout;

}SduRxDecodeCtrlField_t;
/* Private define ------------------------------------------------------------*/
#define SDU_RX_TIMEOUT	50

#define FRAME_ID_TYPE1	2
#define FRAME_ID_TYPE2	3

#define IDX_FRAME_ID_TYPE	0

#define FRAME_CRC_LEN		2
#define FRAME_TAIL_LEN 		1
#define FRAME_TAIL_FIXED_DATA 3
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
osMessageQueueId_t sduQueueHandle;
uint8_t sduQueueBuffer[ 512 * sizeof( uint8_t ) ];
osStaticMessageQDef_t sduQueueControlBlock;
const osMessageQueueAttr_t sduQueue_attributes = {
  .name = "sduQueue",
  .cb_mem = &sduQueueControlBlock,
  .cb_size = sizeof(sduQueueControlBlock),
  .mq_mem = &sduQueueBuffer,
  .mq_size = sizeof(sduQueueBuffer)
};


static osThreadId_t sduProcessTaskHandle;
static const osThreadAttr_t sduProcessTask_attributes = {
  .name = "sduProcessTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static SduRxDecodeCtrlField_t sdu_decode_obj;
/* Private function prototypes -----------------------------------------------*/
static void init(void);
static void run(void *argument);

static void get_frame_id(void);
static void get_packet_length(void);
static void get_packet(void);
static void get_crc(void);
static void get_frame_tail(void);
static void process_commands(void);
static void packet_send_packet(unsigned char *data, unsigned int len) ;
/* Private functions ---------------------------------------------------------*/
void init(void)
{
	sduQueueHandle = osMessageQueueNew (512, sizeof(uint8_t), &sduQueue_attributes);

	sduProcessTaskHandle = osThreadNew(run, NULL, &sduProcessTask_attributes);
	
}

static void run(void *argument)
{
	// uint8_t msg[512];
	// int pos_idx = 0;

	{
		memset(&sdu_decode_obj,0,sizeof(sdu_decode_obj));
		sdu_decode_obj.m_xSduDecodeFrameTask = get_frame_id;
	}


	while (1)
	{
		while(uxQueueMessagesWaiting(sduQueueHandle)>0){
			(*sdu_decode_obj.m_xSduDecodeFrameTask)();
		}
		osDelay(1);
	}
	
}

void packet_send_packet(unsigned char *data, unsigned int len) 
{
	if (len == 0 || len > SDU_FRAME_MAX_LEN-FRAME_ID_TYPE2-FRAME_CRC_LEN-FRAME_TAIL_LEN) {
		return;
	}
	uint8_t _tempBuf[SDU_FRAME_MAX_LEN];

	int b_ind = 0;

	if (len <= 255) {
		_tempBuf[b_ind++] = 2;
		_tempBuf[b_ind++] = len;
	} else if (len <= 65535) {
		_tempBuf[b_ind++] = 3;
		_tempBuf[b_ind++] = len >> 8;
		_tempBuf[b_ind++] = len & 0xFF;
	} else {
		_tempBuf[b_ind++] = 4;
		_tempBuf[b_ind++] = len >> 16;
		_tempBuf[b_ind++] = (len >> 8) & 0xFF;
		_tempBuf[b_ind++] = len & 0xFF;
	}

	memcpy(_tempBuf + b_ind, data, len);
	b_ind += len;

	unsigned short crc = u16UTILS_GetCheckCrc16(data, len);
	_tempBuf[b_ind++] = (uint8_t)(crc >> 8);
	_tempBuf[b_ind++] = (uint8_t)(crc & 0xFF);
	_tempBuf[b_ind++] = 3;

	CDC_Transmit_FS(_tempBuf, b_ind);

}

static void get_frame_id(void)
{
	uint8_t data;
	xQueueReceive(sduQueueHandle, &data, 0);
	if((FRAME_ID_TYPE1==data)||(FRAME_ID_TYPE2==data)){
		sdu_decode_obj.m_xSduDecodeFrameTask = get_packet_length;
		sdu_decode_obj.m_u16SduRxIndicator = IDX_FRAME_ID_TYPE;
		sdu_decode_obj.m_pu8SduRxFrameBuf[sdu_decode_obj.m_u16SduRxIndicator++] =  data;
		sdu_decode_obj.m_xSduTicksForRxFrameTimeout = HAL_GetTick();
	}

}

static void get_packet_length(void)
{
	uint8_t data;
	xQueueReceive(sduQueueHandle, &data, 0);
	sdu_decode_obj.m_pu8SduRxFrameBuf[sdu_decode_obj.m_u16SduRxIndicator++] =  data;	
	if(sdu_decode_obj.m_u16SduRxIndicator==sdu_decode_obj.m_pu8SduRxFrameBuf[IDX_FRAME_ID_TYPE]){
		if(FRAME_ID_TYPE1==sdu_decode_obj.m_pu8SduRxFrameBuf[IDX_FRAME_ID_TYPE]){
			sdu_decode_obj.m_u16SduFrameLength = sdu_decode_obj.m_pu8SduRxFrameBuf[1]+FRAME_ID_TYPE1+FRAME_CRC_LEN+FRAME_TAIL_LEN;
		}else if(FRAME_ID_TYPE2==sdu_decode_obj.m_pu8SduRxFrameBuf[IDX_FRAME_ID_TYPE]){
			sdu_decode_obj.m_u16SduFrameLength = (sdu_decode_obj.m_pu8SduRxFrameBuf[1]<<8)+(sdu_decode_obj.m_pu8SduRxFrameBuf[2])\
			+FRAME_ID_TYPE1+FRAME_CRC_LEN+FRAME_TAIL_LEN;
		}
		sdu_decode_obj.m_xSduDecodeFrameTask = get_packet;
		sdu_decode_obj.m_xSduTicksForRxFrameTimeout = HAL_GetTick();
	}

	if(bUTILS_PeriodicJob(&sdu_decode_obj.m_xSduTicksForRxFrameTimeout,SDU_RX_TIMEOUT)){
		sdu_decode_obj.m_xSduDecodeFrameTask = get_frame_id;
	}
}

static void get_packet(void)
{
	uint8_t data;
	xQueueReceive(sduQueueHandle, &data, 0);
	sdu_decode_obj.m_pu8SduRxFrameBuf[sdu_decode_obj.m_u16SduRxIndicator++] =  data;	
	if(sdu_decode_obj.m_u16SduRxIndicator==sdu_decode_obj.m_u16SduFrameLength-FRAME_CRC_LEN-FRAME_TAIL_LEN){
		sdu_decode_obj.m_xSduDecodeFrameTask = get_crc;
		sdu_decode_obj.m_xSduTicksForRxFrameTimeout = HAL_GetTick();
	}

	if(bUTILS_PeriodicJob(&sdu_decode_obj.m_xSduTicksForRxFrameTimeout,SDU_RX_TIMEOUT*2)){
		sdu_decode_obj.m_xSduDecodeFrameTask = get_frame_id;
	}
}

static void get_crc(void)
{
	uint8_t data;
	xQueueReceive(sduQueueHandle, &data, 0);
	sdu_decode_obj.m_pu8SduRxFrameBuf[sdu_decode_obj.m_u16SduRxIndicator++] =  data;	
	if(sdu_decode_obj.m_u16SduRxIndicator==sdu_decode_obj.m_u16SduFrameLength-FRAME_TAIL_LEN){
		uint16_t crc_get = (sdu_decode_obj.m_pu8SduRxFrameBuf[sdu_decode_obj.m_u16SduFrameLength-FRAME_TAIL_LEN-FRAME_CRC_LEN]<<8)\
		+(sdu_decode_obj.m_pu8SduRxFrameBuf[sdu_decode_obj.m_u16SduFrameLength-FRAME_TAIL_LEN-FRAME_CRC_LEN+1]);
		uint16_t crc_calc = u16UTILS_GetCheckCrc16(sdu_decode_obj.m_pu8SduRxFrameBuf+sdu_decode_obj.m_pu8SduRxFrameBuf[IDX_FRAME_ID_TYPE],\
		sdu_decode_obj.m_u16SduFrameLength-FRAME_TAIL_LEN-FRAME_CRC_LEN-sdu_decode_obj.m_pu8SduRxFrameBuf[IDX_FRAME_ID_TYPE]);
		if(crc_calc==crc_get){
			sdu_decode_obj.m_xSduDecodeFrameTask = get_frame_tail;
			sdu_decode_obj.m_xSduTicksForRxFrameTimeout = HAL_GetTick();
		}else{
			sdu_decode_obj.m_xSduDecodeFrameTask = get_frame_id;			
		}

	}

	if(bUTILS_PeriodicJob(&sdu_decode_obj.m_xSduTicksForRxFrameTimeout,SDU_RX_TIMEOUT*2)){
		sdu_decode_obj.m_xSduDecodeFrameTask = get_frame_id;
	}
}

static void get_frame_tail(void)
{
	uint8_t data;
	xQueueReceive(sduQueueHandle, &data, 0);
	if(FRAME_TAIL_FIXED_DATA==data){
		sdu_decode_obj.m_pu8SduRxFrameBuf[sdu_decode_obj.m_u16SduRxIndicator++] =  data;
		process_commands();
	}
	sdu_decode_obj.m_xSduDecodeFrameTask = get_frame_id;	
}
void process_commands(void)
{
	COMM_PACKET_ID packet_id;

	packet_id = sdu_decode_obj.m_pu8SduRxFrameBuf[sdu_decode_obj.m_pu8SduRxFrameBuf[IDX_FRAME_ID_TYPE]];

	switch (packet_id) {
	case COMM_FW_VERSION: {
		int32_t ind = 0;
		uint8_t send_buffer[65];
		send_buffer[ind++] = COMM_FW_VERSION;
		send_buffer[ind++] = FC_VE_SW_MAJOR;
		send_buffer[ind++] = FC_VE_SW_MINOR;

		strcpy((char*)(send_buffer + ind), FC_VERSION_STRING_HW);
		ind += strlen(FC_VERSION_STRING_HW) + 1;

		memcpy(send_buffer + ind, STM32_UUID_8, 12);
		ind += 12;

		send_buffer[ind++] = 0;//reserve
		send_buffer[ind++] = FC_VE_SW_PATCH_LEVEL;

		send_buffer[ind++] = HW_TYPE_VESC;
		send_buffer[ind++] = 0;//res cfg_num=0...?
		// send_buffer[ind++] = conf_custom_cfg_num();

#ifdef HW_HAS_PHASE_FILTERS
		send_buffer[ind++] = 1;
#else
		send_buffer[ind++] = 0;
#endif

#ifdef QMLUI_SOURCE_HW
#ifdef QMLUI_HW_FULLSCREEN
		send_buffer[ind++] = 2;
#else
		send_buffer[ind++] = 1;
#endif
#else
		send_buffer[ind++] = 0;
#endif

#ifdef QMLUI_SOURCE_APP
#ifdef QMLUI_APP_FULLSCREEN
		send_buffer[ind++] = 2;
#else
		send_buffer[ind++] = 1;
#endif
#else
		send_buffer[ind++] = 0;
#endif
		send_buffer[ind++] = 0;//nrf_flags?

		strcpy((char*)(send_buffer + ind), FC_VERSION_STRING_SW);
		ind += strlen(FC_VERSION_STRING_SW) + 1;

		packet_send_packet(send_buffer, ind);
	} break;

	case COMM_JUMP_TO_BOOTLOADER_ALL_CAN:
		osDelay(100);
	case COMM_JUMP_TO_BOOTLOADER:
		osDelay(100);
		break;

	case COMM_ERASE_NEW_APP_ALL_CAN:
		osDelay(100);
	case COMM_ERASE_NEW_APP: {
		int32_t ind = 0;
		osDelay(100);
		uint8_t send_buffer[50];
		send_buffer[ind++] = COMM_ERASE_NEW_APP;
		send_buffer[ind++] = 1;
		packet_send_packet(send_buffer, ind);
	} break;

	case COMM_WRITE_NEW_APP_DATA_ALL_CAN_LZO:
	case COMM_WRITE_NEW_APP_DATA_ALL_CAN:
		
	case COMM_WRITE_NEW_APP_DATA_LZO:
	case COMM_WRITE_NEW_APP_DATA: {

	} break;

	case COMM_GET_VALUES:
	case COMM_GET_VALUES_SELECTIVE: {

	} break;

	case COMM_SET_DUTY: {

	} break;

	case COMM_SET_CURRENT: {

	} break;

	case COMM_SET_CURRENT_BRAKE: {

	} break;

	case COMM_SET_RPM: {

	} break;

	case COMM_SET_POS: {

	} break;

	case COMM_SET_HANDBRAKE: {

	} break;

	case COMM_SET_DETECT: {

	} break;

	case COMM_SET_SERVO_POS: {

	} break;

	case COMM_SET_MCCONF: {

	} break;

	case COMM_GET_MCCONF:
	case COMM_GET_MCCONF_DEFAULT: {
	
	} break;

	case COMM_SET_APPCONF:
	case COMM_SET_APPCONF_NO_STORE: {

	} break;

	case COMM_GET_APPCONF:
	case COMM_GET_APPCONF_DEFAULT: {
		
	} break;

	case COMM_SAMPLE_PRINT: {

	} break;

	case COMM_REBOOT:
		NVIC_SystemReset();
		break;

	case COMM_ALIVE:

		break;

	case COMM_GET_DECODED_PPM: {

	} break;

	case COMM_GET_DECODED_ADC: {

	} break;

	case COMM_GET_DECODED_CHUK: {

	} break;

	case COMM_GET_DECODED_BALANCE: {

	} break;

	case COMM_FORWARD_CAN: {

	} break;

	case COMM_SET_CHUCK_DATA: {
	
	} break;

	case COMM_CUSTOM_APP_DATA:

		break;

	case COMM_CUSTOM_HW_DATA:

		break;

	case COMM_NRF_START_PAIRING: {

	} break;

	case COMM_GPD_SET_FSW: {
		HUB32_u tmp;
		memcpy(tmp.part,sdu_decode_obj.m_pu8SduRxFrameBuf,4);
		// gpdrive_set_switching_frequency(tmp.fSymbol);
	} break;

	case COMM_GPD_BUFFER_SIZE_LEFT: {

	} break;

	case COMM_GPD_FILL_BUFFER: {

	} break;

	case COMM_GPD_OUTPUT_SAMPLE: {

	} break;

	case COMM_GPD_SET_MODE: {

	} break;

	case COMM_GPD_FILL_BUFFER_INT8: {

	} break;

	case COMM_GPD_FILL_BUFFER_INT16: {

	} break;

	case COMM_GPD_SET_BUFFER_INT_SCALE: {

	} break;

	case COMM_GET_VALUES_SETUP:
	case COMM_GET_VALUES_SETUP_SELECTIVE: {
	
	    } break;

	case COMM_SET_ODOMETER: {

	} break;

	case COMM_SET_MCCONF_TEMP:
	case COMM_SET_MCCONF_TEMP_SETUP: {
	
	} break;

	case COMM_GET_MCCONF_TEMP: {
	
	} break;

	case COMM_EXT_NRF_PRESENT: {
		
	} break;

	case COMM_EXT_NRF_ESB_RX_DATA: {
		
	} break;

	case COMM_SET_BLE_PIN:
	case COMM_SET_BLE_NAME: {

	} break;

	case COMM_APP_DISABLE_OUTPUT: {
		
	} break;

	case COMM_TERMINAL_CMD_SYNC:
	
		break;

	case COMM_GET_IMU_DATA: {
		
	} break;

	case COMM_ERASE_BOOTLOADER_ALL_CAN:
	
	case COMM_ERASE_BOOTLOADER: {
		
	} break;

	case COMM_SET_CURRENT_REL: {
	
	} break;

	case COMM_CAN_FWD_FRAME: {
	
	} break;

	case COMM_SET_BATTERY_CUT: {
		
	} break;

	case COMM_GET_BATTERY_CUT: {

	} break;

	case COMM_SET_CAN_MODE: {
		
	} break;

	case COMM_BMS_GET_VALUES:
	case COMM_BMS_SET_CHARGE_ALLOWED:
	case COMM_BMS_SET_BALANCE_OVERRIDE:
	case COMM_BMS_RESET_COUNTERS:
	case COMM_BMS_FORCE_BALANCE:
	case COMM_BMS_ZERO_CURRENT_OFFSET: {

		break;
	}

	// Power switch
	case COMM_PSW_GET_STATUS: {
	
	} break;

	case COMM_PSW_SWITCH: {
	
	} break;

	case COMM_GET_QML_UI_HW: {
#ifdef QMLUI_SOURCE_HW
		int32_t ind = 0;

		int32_t len_qml = buffer_get_int32(data, &ind);
		int32_t ofs_qml = buffer_get_int32(data, &ind);

		if ((len_qml + ofs_qml) > DATA_QML_HW_SIZE || len_qml > (PACKET_MAX_PL_LEN - 10)) {
			break;
		}

		uint8_t *send_buffer_global = mempools_get_packet_buffer();
		ind = 0;
		send_buffer_global[ind++] = packet_id;
		buffer_append_int32(send_buffer_global, DATA_QML_HW_SIZE, &ind);
		buffer_append_int32(send_buffer_global, ofs_qml, &ind);
		memcpy(send_buffer_global + ind, data_qml_hw + ofs_qml, len_qml);
		ind += len_qml;
		reply_func(send_buffer_global, ind);

		mempools_free_packet_buffer(send_buffer_global);
#endif
	} break;

	case COMM_GET_QML_UI_APP:
	case COMM_LISP_READ_CODE: {


#ifdef QMLUI_SOURCE_APP
		qmlui_data = data_qml_app;
		qmlui_len = DATA_QML_APP_SIZE;
#endif

	} break;

	case COMM_QMLUI_ERASE:
	case COMM_LISP_ERASE_CODE: {

#ifdef USE_LISPBM
		if (packet_id == COMM_LISP_ERASE_CODE) {
			lispif_restart(false, false);
		}
#endif

	
	} break;

	case COMM_QMLUI_WRITE:
	case COMM_LISP_WRITE_CODE: {
		
	} break;

	case COMM_IO_BOARD_GET_ALL: {
		
	} break;

	case COMM_IO_BOARD_SET_PWM: {

	} break;

	case COMM_IO_BOARD_SET_DIGITAL: {

	} break;

	case COMM_GET_STATS: {
		
	} break;

	case COMM_RESET_STATS: {
	
	} break;

	case COMM_GET_GNSS: {
		
	} break;

	case COMM_LISP_SET_RUNNING:
	case COMM_LISP_GET_STATS:
	case COMM_LISP_REPL_CMD:
	case COMM_LISP_STREAM_CODE: {
#ifdef USE_LISPBM
		lispif_process_cmd(data - 1, len + 1, reply_func);
#endif
		break;
	}

	case COMM_GET_CUSTOM_CONFIG:
	case COMM_GET_CUSTOM_CONFIG_DEFAULT:
	case COMM_SET_CUSTOM_CONFIG:
	case COMM_GET_CUSTOM_CONFIG_XML: {

	} break;

	// Blocking commands. Only one of them runs at any given time, in their
	// own thread. If other blocking commands come before the previous one has
	// finished, they are discarded.
	case COMM_TERMINAL_CMD:
	case COMM_DETECT_MOTOR_PARAM:
	case COMM_DETECT_MOTOR_R_L:
	case COMM_DETECT_MOTOR_FLUX_LINKAGE:
	case COMM_DETECT_ENCODER:
	case COMM_DETECT_HALL_FOC:
	case COMM_DETECT_MOTOR_FLUX_LINKAGE_OPENLOOP:
	case COMM_DETECT_APPLY_ALL_FOC:
	case COMM_PING_CAN:
	case COMM_BM_CONNECT:
	case COMM_BM_ERASE_FLASH_ALL:
	case COMM_BM_WRITE_FLASH_LZO:
	case COMM_BM_WRITE_FLASH:
	case COMM_BM_REBOOT:
	case COMM_BM_DISCONNECT:
	case COMM_BM_MAP_PINS_DEFAULT:
	case COMM_BM_MAP_PINS_NRF5X:
	case COMM_BM_MEM_READ:
	case COMM_GET_IMU_CALIBRATION:
	case COMM_BM_MEM_WRITE:

		break;

	default:
		break;
	}

}


/* Exported functions --------------------------------------------------------*/
SDU_t sdu = {
	.m_xInitialize = init,
};