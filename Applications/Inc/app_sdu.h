/* Define to prevent recursive inclusion -------------------------------------*/
#pragma once
/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "queue.h"
#include "main.h"
#include "cmsis_os.h"

#include "app_utils.h"
/* Exported types ------------------------------------------------------------*/

typedef struct SDU_OBJ{
    V_NULL_FUNCTION_SYMBOL m_xInitialize;//SDU module initial.
}SDU_t;
/* Exported constants --------------------------------------------------------*/
#define SDU_FRAME_MAX_LEN 512
/* Exported macros -----------------------------------------------------------*/


/* Exported functions --------------------------------------------------------*/
extern SDU_t sdu;