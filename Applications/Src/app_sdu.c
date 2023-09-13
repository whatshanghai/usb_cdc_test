/* Includes ------------------------------------------------------------------*/
#include "app_sdu.h"

#include "usbd_cdc_if.h"
/* Private typedef -----------------------------------------------------------*/
typedef StaticQueue_t osStaticMessageQDef_t;

typedef struct SDU_RxDecodeCtrlField {

	uint8_t m_pu8SduRxFrameBuf[SDU_FRAME_MAX_LEN];
	uint8_t m_pu8SduTxFrameBuf[SDU_FRAME_MAX_LEN];

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
/* Private functions ---------------------------------------------------------*/
void init(void)
{
	sduQueueHandle = osMessageQueueNew (512, sizeof(uint8_t), &sduQueue_attributes);

	sduProcessTaskHandle = osThreadNew(run, NULL, &sduProcessTask_attributes);
	
}

static void run(void *argument)
{
	uint8_t msg[512];
	int pos_idx = 0;

	{
		memset(&sdu_decode_obj,0,sizeof(sdu_decode_obj));
		sdu_decode_obj.m_xSduDecodeFrameTask = get_frame_id;
	}


	while (1)
	{
		pos_idx = 0;
		while(uxQueueMessagesWaiting(sduQueueHandle)>0){
			xQueueReceive(sduQueueHandle, msg+pos_idx, 0);
			++pos_idx;

			(*sdu_decode_obj.m_xSduDecodeFrameTask)();
		}
		CDC_Transmit_FS(msg,pos_idx);
		osDelay(1);
	}
	
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
			sdu_decode_obj.m_u16SduFrameLength = sdu_decode_obj.m_pu8SduRxFrameBuf[1]+(sdu_decode_obj.m_pu8SduRxFrameBuf[2]<<8)\
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
		uint16_t crc_calc = sdu_decode_obj.m_pu8SduRxFrameBuf[sdu_decode_obj.m_u16SduFrameLength-FRAME_TAIL_LEN-FRAME_CRC_LEN-1]\
		+(sdu_decode_obj.m_pu8SduRxFrameBuf[sdu_decode_obj.m_u16SduFrameLength-FRAME_TAIL_LEN-FRAME_CRC_LEN]<<8);
		uint16_t crc_get = u16UTILS_GetCheckCrc(sdu_decode_obj.m_pu8SduRxFrameBuf,sdu_decode_obj.m_u16SduFrameLength-FRAME_TAIL_LEN-FRAME_CRC_LEN);
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
		//parse interface.
	}
	sdu_decode_obj.m_xSduDecodeFrameTask = get_frame_id;	
}



/* Exported functions --------------------------------------------------------*/
SDU_t sdu = {
	.m_xInitialize = init,
};