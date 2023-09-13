
/* Define to prevent recursive inclusion -------------------------------------*/
#pragma once
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "stdbool.h"
#include "string.h"
#include "main.h"
//#include "SEGGER_RTT.h"
//#include "SEGGER_RTT_Conf.h"
/* Exported types ------------------------------------------------------------*/
/*
CIA Baudrate
75% 	>800K
80% 	>500K
87.5% 	<=500K

sample =(1+TSEG1)/(1+TSEG1+TSEG2)
*/

/*can clock 80MHz
#define CAN_BAUDRATE_NUM    8

const uint32_t prescalerBuf[CAN_BAUDRATE_NUM]={10,20,32,40,
  64,80,160,800};
const uint32_t timeSeg1Buf[CAN_BAUDRATE_NUM]={CAN_BS1_5TQ,CAN_BS1_6TQ,CAN_BS1_8TQ,CAN_BS1_8TQ,
  CAN_BS1_8TQ,CAN_BS1_8TQ,CAN_BS1_8TQ,CAN_BS1_8TQ};
const uint32_t timeSeg2Buf[CAN_BAUDRATE_NUM]={CAN_BS2_2TQ,CAN_BS2_1TQ,CAN_BS2_1TQ,CAN_BS2_1TQ,
  CAN_BS2_1TQ,CAN_BS2_1TQ,CAN_BS2_1TQ,CAN_BS2_1TQ}; 
*/	

typedef  uint32_t Timer_t;



typedef void(*V_FUCTION_V_SYMBOL)(void);
typedef bool(*B_NULL_FUNCTION_SYMBOL)(void);
typedef void(*V_NULL_FUNCTION_SYMBOL)(void);
typedef void(*V_V_FUNCTION_SYMBOL)( void *);
typedef void(*V_FUNCTION_U8_SYMBOL)(uint8_t data);
typedef void(*V_FUNCTION_PU8_SYMBOL)(uint8_t * p_data);
typedef void(*V_FUNCTION_U8_I32_SYMBOL)(uint8_t data1,int32_t data2);
typedef void(*V_FUNCTION_U8_FLOAT_SYMBOL)(uint8_t data1,float data2);
typedef void(*V_FUNCTION_U8_BOOL_SYMBOL)(uint8_t data1,bool data2);
/** 
  * @brief  eBool definition  
  */
typedef enum 
{
  bFalse = 0x00U,
  bTrue  = 0x01U  
} eBool;

typedef union HUB32
{
    uint32_t u32Symbol;
    int32_t i32Symbol;
    float 	fSymbol;
    uint8_t  part[4];
}HUB32_u;

typedef union HUB16
{
    uint16_t u16Symbol;
    int16_t i16Symbol;
    uint8_t  part[2];
}HUB16_u;

//打印信息级别
typedef enum
{
    __NULL__ = 0,                               //空
    __DEBUG__,                                  //调试,用于调试打印所用的详细信息
    __INFO__,                                   //信息,提供系统运行、系统操作的一般信息
    __WARN__,                                   //警告,系统出现轻微的不合理状况,但不影响运行和使用
    __ERROR__,                                  //错误,系统出现错误和异常,可能无法正常完成目标操作
    __FATAL__                                   //严重错误,系统出现了严重错误和异常,系统不能正常工作
}LOG_PRINT_TYPE;




/* Exported constants --------------------------------------------------------*/
#define SW_VERSION_MAIN    1
#define SW_VERSION_SUB     0

extern const uint8_t BitReverseTable[];
/* Exported macros -----------------------------------------------------------*/
//打印数据最大长度,根据MCU内存大小调整此数值
#define LOG_PRINT_MAXSTRLEN	    (512)           //由用户自定义,默认为512

//是否需要彩色打印输出(1表示需要,0表示不需要)
#define LOG_PRINT_COLOR_SUPPORT    (1)             //由用户自定义,默认为0.(注:此功能与终端有关系,若终端不支持彩色插件功能,则即使配置为1也不会有彩色输出)

//#define PRINT             SEGGER_RTT_printf

#define QUICK_MSB(x)      (((x) >> 8) & 0xff)                              /* most significant byte of 2-byte integer.	 */
#define QUICK_LSB(x)      ((x) & 0xff)                                     /* least significant byte of 2-byte integer.	 */

#define QUICK_MSW(x)      (((x) >> 16) & 0xffff)
#define QUICK_LSW(x)      ((x) & 0xffff)

#define COMBINE_2BYTE_BY_L(x,y) ((x+(y<<8))&0xffff)
#define COMBINE_2BYTE_BY_M(x,y) ((y+(x<<8))&0xffff)

#define DIAMOND     	  while(1)

#define QUICK_MAX(x, y)   (((x) < (y)) ? (y) : (x))
#define QUICK_MIN(x, y)   (((x) < (y)) ? (x) : (y))

#define QUICK_ABS(X)	  (((X)<0)?-(X):(X))


#define NOOP do {} while (0)

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))
#define ARRAYEND(x) (&(x)[ARRAYLEN(x)])

#define CONST_CAST(type, value) ((type)(value))

#define CONCAT_HELPER(x,y) x ## y
#define CONCAT(x,y) CONCAT_HELPER(x, y)
#define CONCAT2(_1,_2) CONCAT(_1, _2)
#define CONCAT3(_1,_2,_3)  CONCAT(CONCAT(_1, _2), _3)
#define CONCAT4(_1,_2,_3,_4)  CONCAT(CONCAT3(_1, _2, _3), _4)

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define EXPAND_I(x) x
#define EXPAND(x) EXPAND_I(x)

// expand to t if bit is 1, f when bit is 0. Other bit values are not supported
#define PP_IIF(bit, t, f) PP_IIF_I(bit, t, f)
#define PP_IIF_I(bit, t, f) PP_IIF_ ## bit(t, f)
#define PP_IIF_0(t, f) f
#define PP_IIF_1(t, f) t

// Expand all argumens and call macro with them. When expansion of some argument contains ',', it will be passed as multiple arguments
// #define TAKE3(_1,_2,_3) CONCAT3(_1,_2,_3)
// #define MULTI2 A,B
// PP_CALL(TAKE3, MULTI2, C) expands to ABC
#define PP_CALL(macro, ...) macro(__VA_ARGS__)

#if !defined(UNUSED)
#define UNUSED(x) (void)(x) // Variables and parameters that are not used
#endif

#define DISCARD(x) (void)(x) // To explicitly ignore result of x (usually an I/O register access).

#define STATIC_ASSERT(condition, name) _Static_assert((condition), #name)


#define BIT(x) (1 << (x))

/*
http://resnet.uoregon.edu/~gurney_j/jmpc/bitwise.html
*/
#define BITCOUNT(x) (((BX_(x)+(BX_(x)>>4)) & 0x0F0F0F0F) % 255)
#define BX_(x) ((x) - (((x)>>1)&0x77777777) - (((x)>>2)&0x33333333) - (((x)>>3)&0x11111111))

#define ENTER_CRITICAL()		__ASM volatile("cpsid i")
#define EXIT_CRITICAL()			__ASM volatile("cpsie i")
/* Exported functions --------------------------------------------------------*/
void vUTILS_LOG_PRINT_LEVEL_SET(LOG_PRINT_TYPE level);       //设置打印信息级别(低于此等级,则不打印,默认是所有信息均打印)


/**
 * @description                 : 打印模块,可根据信息级别进行打印输出.
 * @param  {unsigned char type} ：打印信息级别
 * @param  {const char *format} ：打印信息内容,与printf使用方法一致
 * @return {void} : None
 */
void vUTILS_LOG_PRINT(LOG_PRINT_TYPE type, const char* format, ...);

uint8_t u8UTILS_BitReverse(uint8_t data);
uint32_t u32UTILS_BitReverse(uint32_t data);


uint8_t u8UTILS_GetCheckSum(uint8_t *pBuf,uint16_t len);
uint8_t u8UTILS_GetCheckXor(uint8_t *pBuf,uint16_t len);

uint16_t u16UTILS_GetCheckSum(uint16_t *pBuf,uint16_t len);
uint16_t u16UTILS_GetCheckXor(uint16_t *pBuf,uint16_t len);
uint16_t u16UTILS_GetCheckCrc(uint8_t *puchMsg, uint16_t usDataLen) ;

uint32_t u32UTILS_GetCheckSum(uint32_t *pBuf,uint32_t len);
uint32_t u32UTILS_GetCheckXor(uint32_t *pBuf,uint32_t len);


__STATIC_INLINE bool bUTILS_PeriodicJob(Timer_t *pTickRcdr, Timer_t cycleMs)
{
	Timer_t tmpTick;
    ENTER_CRITICAL();
    tmpTick = HAL_GetTick();
    EXIT_CRITICAL();
    if ((tmpTick-*pTickRcdr)>=cycleMs){
        *pTickRcdr = tmpTick;
        return true;
    }

    return false;
}

__STATIC_INLINE bool bUTILS_OneShotJob(Timer_t tickRcdr, Timer_t *duration)
{
	Timer_t tmpTick;
    ENTER_CRITICAL();
    tmpTick = HAL_GetTick();
    EXIT_CRITICAL();    
    if((tmpTick-tickRcdr) >= *duration){
      *duration = HAL_MAX_DELAY;
      return true;
    }else{
      return false;    
    }
}
