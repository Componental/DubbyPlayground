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
//#include "debug.h"

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
#define debugLED(type, ...)				printDEBUG(type, (char *)c_module_name, __VA_ARGS__)

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Private function prototypes
//------------------------------------------------------------------------------

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Private variables
//------------------------------------------------------------------------------
//volatile LED_INFO g_LED_INFO;
//volatile LED_INFO _attribute_section((section("._stext"))) g_LED_INFO; 
__attribute__((section(".sram1_bss"))) volatile LED_INFO g_LED_INFO;

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
	GPIOC->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED10);
	GPIOC->MODER |= (GPIO_MODER_MODE10_0);

	{
		DMAMUX1_Channel7->CCR = MUX_TIM7_UP;

		RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
		__DMB();

		DMA1_Stream7->CR = 0x00000000;
		__DMB();
		
		DMA1_Stream7->PAR = (uint32_t)&GPIOC->BSRR;
		DMA1_Stream7->M0AR = (uint32_t)g_LED_INFO.tx_buff; 
		DMA1_Stream7->NDTR = LED_DMA_NMBR_OF_TRANS;

		DMA1_Stream7->CR |= (DMA_SxCR_PL | DMA_SxCR_TCIE | DMA_SxCR_MSIZE_1 | DMA_SxCR_PSIZE_1);
		DMA1_Stream7->CR |= DMA_SxCR_DIR_0;
		DMA1_Stream7->CR |= DMA_SxCR_MINC;
	}

	{
		RCC->APB1LENR |= RCC_APB1LENR_TIM7EN;
		__DMB();
		TIM7->PSC = 14-1;
		TIM7->ARR = 3-1;

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
	
 	//debugLED(DNONE, "init done\n");
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
	DMA1_Stream7->CR &= ~DMA_SxCR_EN;
	DMA1_Stream7->NDTR = LED_DMA_NMBR_OF_TRANS;
	
	g_LED_INFO.tx_cnt = LED_DATA_CNT_OFFSET;
	uint32_t k = 0;

	for(k=0;k<LED_NUMBER;k++)
	{
		uint32_t j = 0;
		uint32_t i = 0;
		uint32_t led_buffer = g_LED_INFO.led_buffer[k];
		for(j=0;j<24;j++)
		{
			if (led_buffer & 0x00800000)
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
			led_buffer = (led_buffer << 1);
		}
	}
	DMA1->HIFCR = (DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7 | DMA_HIFCR_CFEIF7 | DMA_HIFCR_CDMEIF7 | DMA_HIFCR_CTEIF7);
	DMA1_Stream7->CR |= DMA_SxCR_EN;
	TIM7->CR1 |= TIM_CR1_CEN;
}

void chkLED(void)
{
	
}


//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Private functions
//------------------------------------------------------------------------------


