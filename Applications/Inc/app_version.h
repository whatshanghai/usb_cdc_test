#pragma once

#include "app_utils.h"
#include "string.h"

typedef struct
{
    uint8_t year;
    uint8_t month;
    uint8_t date;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    char * pSwVerStr;
    char * pHwVerStr;
    char * pCompileTime;
    char * pFirmwareName;
    uint32_t sn[3];
}CompileVer_t,*CompileVer_pt;

#define FC_FIRMWARE_NAME            "DroidSurg_CC"
#define FC_HARDWARE_NAME            "XDRV_TERRY"

#define FC_VE_SW_MAJOR            6  // increment when a major release is made (big new feature, etc)
#define FC_VE_SW_MINOR            2  // increment when a minor release is made (small new feature, change etc)
#define FC_VE_SW_PATCH_LEVEL      0  // increment when a bug is fixed

#define FC_VER_HW_MAJOR         1
#define FC_VER_HW_MINOR         0
#define FC_VER_HW_PATCH_LEVEL   0

#define FC_VERSION_STRING_SW FC_FIRMWARE_NAME" "STR(FC_VE_SW_MAJOR) "." STR(FC_VE_SW_MINOR) "." STR(FC_VE_SW_PATCH_LEVEL)
#define FC_VERSION_STRING_HW FC_HARDWARE_NAME" "STR(FC_VER_HW_MAJOR) "." STR(FC_VER_HW_MINOR) "." STR(FC_VER_HW_PATCH_LEVEL)


#define BUILD_DATE_LENGTH 11
extern const char* const buildDate;  // "MMM DD YYYY" MMM = Jan/Feb/...

#define BUILD_TIME_LENGTH 8
extern const char* const buildTime;  // "HH:MM:SS"

#define FC_COMPILE_TIME	   __DATE__" "__TIME__

#define STM32_UUID					((uint32_t*)UID_BASE)
#define STM32_UUID_8				((uint8_t*)UID_BASE)
/* Exported functions --------------------------------------------------------*/
void vVER_Init(void);
CompileVer_pt xVER_GetCompileInfo(void);
