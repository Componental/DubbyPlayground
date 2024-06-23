///wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
/// @file        led.h
/// @author      IRQdesign d.o.o Tuzla
///              Bosnia and Herzegovina
///              www.irqdesign.com
/// @date        Nov 15 2023
/// @version     1.0.0
///wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww

#ifndef __LED_H__
#define __LED_H__

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Includes
//------------------------------------------------------------------------------
#include <stdint.h>
#include <stdarg.h>
// #include "mcuid.h"
// #include "debug.h"
// #include "delay.h"

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Module configuration
//------------------------------------------------------------------------------

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Exported constants
//------------------------------------------------------------------------------

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Exported macros
//------------------------------------------------------------------------------
#define MUX_TIM3_UP					27
#define MUX_TIM7_UP					70
#define LED_DMA_NMBR_OF_TRANS		1776
#define LED_NUMBER					2
#define LED_DATA_CNT_OFFSET			600

#define WHITE					0x00FFFFFF
#define RED						0x0000FF00
#define BLUE					0x000000FF
#define RAND_COLOR				0x0088FF88
#define GREEN					0x00FF0000
#define NAVY					0x00000080
#define PURPLE					0x00008080
#define YELLOW					0x00FFFF00
#define NO_COLOR				0x00000000
//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Exported types
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t status;
    uint8_t state;
    uint32_t timer;
	
	uint32_t red_color;
	uint32_t green_color;
	uint32_t blue_color;
	uint32_t tx_cnt;
	uint32_t tx_buff[LED_DMA_NMBR_OF_TRANS];
	//uint32_t * tx_buff;
	uint32_t led_buffer[LED_NUMBER];
} LED_INFO;

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Exported variables
//------------------------------------------------------------------------------
extern volatile LED_INFO g_LED_INFO;

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Exported functions
//------------------------------------------------------------------------------

void initLED(void);
void chkLED(void);
void updateLED(void);
void setLED(uint8_t led_id, uint32_t color, uint32_t brightness);

#endif 



