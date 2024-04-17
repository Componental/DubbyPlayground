///wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
/// @file        led.c
/// @author      IRQdesign d.o.o Tuzla
///              Bosnia and Herzegovina
///              www.irqdesign.com
/// @date        Nov 15 2023
/// @version     1.0.0
/// @brief       
///
///-----------------------------------------------------------------------------
/// NOTES:
/// 
///wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
#include "led.h"

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// An example of module application
//------------------------------------------------------------------------------
//
//
//
//
//
//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Private types
//------------------------------------------------------------------------------

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Private constants
//------------------------------------------------------------------------------

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Private macros
//------------------------------------------------------------------------------
static const uint8_t c_module_name[] = "LED";
//#define debugLED(type, ...)				printDEBUG(type, (char *)c_module_name, __VA_ARGS__)

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Private function prototypes
//------------------------------------------------------------------------------

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Private variables
//------------------------------------------------------------------------------
volatile LED_INFO g_LED_INFO;

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Exported functions
//------------------------------------------------------------------------------
void initLED(void)
{
	uint32_t k;
	g_LED_INFO.tx_cnt = 0;

	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOCEN;
	__DMB();
	GPIOC->MODER &= ~(GPIO_MODER_MODE10);
	GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD10);
	GPIOC->MODER |= (GPIO_MODER_MODE10_0);

	{
		DMAMUX1_Channel1->CCR = MUX_TIM7_UP;

		RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
		__DMB();

		DMA1_Stream1->CR = 0x00000000;
		while((DMA1_Stream1->CR & RCC_AHB1ENR_DMA1EN) == RCC_AHB1ENR_DMA1EN);

		DMA1_Stream1->PAR = (uint32_t)&GPIOC->BSRR;
		DMA1_Stream1->M0AR = (uint32_t)g_LED_INFO.tx_buff; 
		DMA1_Stream1->NDTR = LED_DMA_NMBR_OF_TRANS;

		DMA1_Stream1->CR |= (DMA_SxCR_PL | DMA_SxCR_TCIE | DMA_SxCR_MSIZE_1 | DMA_SxCR_PSIZE_1);
		DMA1_Stream1->CR |= DMA_SxCR_DIR_0;
		DMA1_Stream1->CR |= DMA_SxCR_MINC;
	}

	{
		RCC->APB1LENR |= RCC_APB1LENR_TIM7EN;
		__DMB();
		TIM7->PSC = 6-1;
		TIM7->ARR = 3;

		TIM7->CR1 = 0x0084;
		TIM7->CR2 = 0x0000;
		TIM7->CR2 |= TIM_CR2_MMS2_1;
		TIM7->DIER |= (TIM_DIER_UDE);
		TIM7->CNT = 0x0000;
		TIM7->EGR |= TIM_EGR_UG;
	}
	for(k=0;k<600;k++)
	{
		g_LED_INFO.tx_buff[g_LED_INFO.tx_cnt] = GPIO_BSRR_BR10;
		g_LED_INFO.tx_cnt++;
	}
	setLED(0, NO_COLOR, 0);
	setLED(1, NO_COLOR, 0);
	updateLED();
	for(k=0;k<600;k++)
	{
		g_LED_INFO.tx_buff[g_LED_INFO.tx_cnt] = GPIO_BSRR_BR10;
		g_LED_INFO.tx_cnt++;
	}
	
// #define LED_UNIT_TEST
// #ifdef LED_UNIT_TEST
// 	debugLED(DWARNING, "LED UNIT TEST\n");
// 	uint32_t timer = 0;
// 	uint32_t n = 1;
// 	uint32_t pause_ms = 50;
// 	uint8_t mode = 0;
// 	while(1)
// 	{
// 		if(mode == 0)
// 		{
// 			mode = 1;
// 			timer = getSYSTIM();
// 			if(n == 1)
// 			{
// 				debugLED(DNONE, "led0: red  led1: blue\n");
// 				setLED(0, RED, 100);
// 				setLED(1, BLUE, 100);
// 				updateLED();
// 			}
// 			else if(n == 2)
// 			{
// 				debugLED(DNONE, "led0: green  led1: yellow\n");
// 				setLED(0, GREEN, 20);
// 				setLED(1, YELLOW, 20);
// 				updateLED();
// 			}
// 			else if(n == 3)
// 			{
// 				debugLED(DNONE, "led0: blue  led1: red\n");
// 				setLED(0, BLUE, 1);
// 				setLED(1, RED, 1);
// 				updateLED();
// 			}
// 			else if(n == 4)
// 			{
// 				debugLED(DNONE, "led0: blue  led1: off\n");
// 				setLED(0, BLUE, 100);
// 				setLED(1, NO_COLOR, 0);
// 				updateLED();
// 			}
// 			else if(n == 5)
// 			{
// 				debugLED(DNONE, "led0: off  led1: red\n");
// 				setLED(0, NO_COLOR, 0);
// 				setLED(1, RED, 100);
// 				updateLED();
// 			}
// 			else
// 			{
// 				debugLED(DNONE, "shut down\n");
// 				setLED(0, NO_COLOR, 0);
// 				setLED(1, NO_COLOR, 0);
// 				updateLED();
// 				n = 0;

// 			}
// 		}
// 		else
// 		{
// 			if(chk4TimeoutSYSTIM(timer, pause_ms) == (SYSTIM_TIMEOUT))
// 			{
// 				mode = 0;
// 				n++;
// 			}
// 		}
// 	}
// #endif

// 	debugLED(DNONE, "init done\n");
}

void setLED(uint8_t led_id, uint32_t color, uint32_t brightness)
{
	if(led_id < 0 || led_id > LED_NUMBER)
	{
		// debugLED(DERROR, "led id out of range\n");
		return;
	}
	if(brightness > 100 || brightness < 0)
	{
		// debugLED(DERROR, "brightness out of range\n");
		return;
	}

	g_LED_INFO.red_color = ((color >> 16) & 0x000000FF);
	g_LED_INFO.green_color = ((color >> 8) & 0x000000FF);
	g_LED_INFO.blue_color = (color & 0x000000FF);

	g_LED_INFO.red_color = (g_LED_INFO.red_color * brightness)/100;
	g_LED_INFO.green_color = (g_LED_INFO.green_color * brightness)/100;
	g_LED_INFO.blue_color = (g_LED_INFO.blue_color * brightness)/100;

	color = (0xFF000000 | (g_LED_INFO.red_color << 16) | (g_LED_INFO.green_color << 8) | g_LED_INFO.blue_color);
	g_LED_INFO.led_buffer[led_id] = color;
}

void updateLED(void)
{
	g_LED_INFO.tx_cnt = LED_DATA_CNT_OFFSET;
	uint32_t k = 0;

	for(k=0;k<LED_NUMBER;k++)
	{
		uint32_t j = 0;
		uint32_t i = 0;

		for(j=0;j<24;j++)
		{
			if ((g_LED_INFO.led_buffer[k] & 0x1) == 0x1)
			{
				for (i=0;i<12;i++)
				{
					if (i < 6)
					{
						g_LED_INFO.tx_buff[g_LED_INFO.tx_cnt] = GPIO_BSRR_BS10;
					}
					else
					{
						g_LED_INFO.tx_buff[g_LED_INFO.tx_cnt] = GPIO_BSRR_BR10;
					}
					++g_LED_INFO.tx_cnt;
				}
			}
			else
			{
				for (i=0;i<12;i++)
				{
					if (i < 2)
					{
						g_LED_INFO.tx_buff[g_LED_INFO.tx_cnt] = GPIO_BSRR_BS10;
					}
					else
					{
						g_LED_INFO.tx_buff[g_LED_INFO.tx_cnt] = GPIO_BSRR_BR10;
					}
					++g_LED_INFO.tx_cnt;
				}
			}
			g_LED_INFO.led_buffer[k] = (g_LED_INFO.led_buffer[k] >> 1);
		}
	}
	DMA1->LIFCR = (DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CFEIF1 | DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CTEIF1);
	DMA1_Stream1->CR |= DMA_SxCR_EN;
	TIM7->CR1 |= TIM_CR1_CEN;
}

void chkLED(void)
{
}


//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Private functions
//------------------------------------------------------------------------------


