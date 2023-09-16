#include "app_version.h"
#include "app_utils.h"

/* Private macro -------------------------------------------------------------*/
#define COMPUTE_BUILD_YEAR \
    ( \
        (buildDate[ 9] - '0') *   10 + \
        (buildDate[10] - '0') \
    )

#define COMPUTE_BUILD_DAY \
    ( \
        ((buildDate[4] >= '0') ? (buildDate[4] - '0') * 10 : 0) + \
        (buildDate[5] - '0') \
    )

#define BUILD_MONTH_IS_JAN (buildDate[0] == 'J' && buildDate[1] == 'a' && buildDate[2] == 'n')
#define BUILD_MONTH_IS_FEB (buildDate[0] == 'F')
#define BUILD_MONTH_IS_MAR (buildDate[0] == 'M' && buildDate[1] == 'a' && buildDate[2] == 'r')
#define BUILD_MONTH_IS_APR (buildDate[0] == 'A' && buildDate[1] == 'p')
#define BUILD_MONTH_IS_MAY (buildDate[0] == 'M' && buildDate[1] == 'a' && buildDate[2] == 'y')
#define BUILD_MONTH_IS_JUN (buildDate[0] == 'J' && buildDate[1] == 'u' && buildDate[2] == 'n')
#define BUILD_MONTH_IS_JUL (buildDate[0] == 'J' && buildDate[1] == 'u' && buildDate[2] == 'l')
#define BUILD_MONTH_IS_AUG (buildDate[0] == 'A' && buildDate[1] == 'u')
#define BUILD_MONTH_IS_SEP (buildDate[0] == 'S')
#define BUILD_MONTH_IS_OCT (buildDate[0] == 'O')
#define BUILD_MONTH_IS_NOV (buildDate[0] == 'N')
#define BUILD_MONTH_IS_DEC (buildDate[0] == 'D')


#define COMPUTE_BUILD_MONTH \
    ( \
        (BUILD_MONTH_IS_JAN) ?  1 : \
        (BUILD_MONTH_IS_FEB) ?  2 : \
        (BUILD_MONTH_IS_MAR) ?  3 : \
        (BUILD_MONTH_IS_APR) ?  4 : \
        (BUILD_MONTH_IS_MAY) ?  5 : \
        (BUILD_MONTH_IS_JUN) ?  6 : \
        (BUILD_MONTH_IS_JUL) ?  7 : \
        (BUILD_MONTH_IS_AUG) ?  8 : \
        (BUILD_MONTH_IS_SEP) ?  9 : \
        (BUILD_MONTH_IS_OCT) ? 10 : \
        (BUILD_MONTH_IS_NOV) ? 11 : \
        (BUILD_MONTH_IS_DEC) ? 12 : \
        /* error default */  99 \
    )

#define COMPUTE_BUILD_HOUR ((buildTime[0] - '0') * 10 + buildTime[1] - '0')
#define COMPUTE_BUILD_MIN  ((buildTime[3] - '0') * 10 + buildTime[4] - '0')
#define COMPUTE_BUILD_SEC  ((buildTime[6] - '0') * 10 + buildTime[7] - '0')


#define BUILD_DATE_IS_BAD (buildDate[0] == '?')
#define BUILD_TIME_IS_BAD (buildTime[0] == '?')
 

/* private constants --------------------------------------------------------*/

const char * const buildDate = __DATE__;
const char * const buildTime = __TIME__;

/* private variables --------------------------------------------------------*/
static CompileVer_t cpVersion;


/* Private function prototypes -----------------------------------------------*/
static void  updateBuildInfo(void);
static uint8_t  getBuildYear(void);
static uint8_t  getBuildMonth(void);
static uint8_t  getBuildDay(void);
static uint8_t  getBuildHour(void);
static uint8_t  getBuildMin(void);
static uint8_t  getBuildSec(void);

/* Private functions ---------------------------------------------------------*/
void  updateBuildInfo(void)
{
    cpVersion.pSwVerStr = FC_VERSION_STRING_SW;
    cpVersion.pHwVerStr = FC_VERSION_STRING_HW;
    cpVersion.pFirmwareName = FC_FIRMWARE_NAME;
    cpVersion.pCompileTime = FC_COMPILE_TIME;
    cpVersion.sn[0]=HAL_GetUIDw0();
    cpVersion.sn[1]=HAL_GetUIDw1();
    cpVersion.sn[2]=HAL_GetUIDw2();
}
      
uint8_t  getBuildYear()
{
    return ((BUILD_DATE_IS_BAD) ? 99 : COMPUTE_BUILD_YEAR);
}

uint8_t  getBuildMonth()
{
    return ((BUILD_DATE_IS_BAD) ? 99 : COMPUTE_BUILD_MONTH);
}
uint8_t  getBuildDay()
{
    return ((BUILD_DATE_IS_BAD) ? 99 : COMPUTE_BUILD_DAY);
}
uint8_t  getBuildHour()
{
    return ((BUILD_TIME_IS_BAD) ? 99 :  COMPUTE_BUILD_HOUR);
}
uint8_t  getBuildMin()
{
    return ((BUILD_TIME_IS_BAD) ? 99 :  COMPUTE_BUILD_MIN);
}
uint8_t  getBuildSec()
{
    return ((BUILD_TIME_IS_BAD) ? 99 :  COMPUTE_BUILD_SEC);
}
/* Exported functions --------------------------------------------------------*/

void vVER_Init(void)
{
	updateBuildInfo();
    cpVersion.month = getBuildMonth();
    cpVersion.date = getBuildDay();
    cpVersion.year = getBuildYear();
    cpVersion.hour = getBuildHour();
    cpVersion.minute = getBuildMin();
    cpVersion.second = getBuildSec();
}

CompileVer_pt xVER_GetCompileInfo(void)
{
	return &cpVersion;
}
