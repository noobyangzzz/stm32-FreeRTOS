/**
  ******************************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2013-xx-xx
  * @brief   SysTick系统滴答定时器测试
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火 F103-指南者 STM32 开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */ 

#include "stm32f10x.h"
#include "bsp_led.h"
#include "bsp_usart.h"
#include "bsp_systick.h"

static void Usart_SendNumber(uint32_t number)
{
	char buffer[11];
	int i = 0;

	if (number == 0)
	{
		Usart_SendByte(DEBUG_USARTx, '0');
		return;
	}

	while (number > 0)
	{
		buffer[i++] = (char)('0' + (number % 10));
		number /= 10;
	}

	while (i > 0)
	{
		Usart_SendByte(DEBUG_USARTx, buffer[--i]);
	}
}

static void Usart_SendTick(uint32_t tick)
{
	Usart_SendString(DEBUG_USARTx, "tick = ");
	Usart_SendNumber(tick);
	Usart_SendString(DEBUG_USARTx, " ms\r\n");
}


/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */ 
int main(void)
{
	LED_GPIO_Config();
	LED_RGBOFF;

	USART_Config();
	Usart_SendString(DEBUG_USARTx, "lesson-04-systick start\r\n");

	SysTick_Init();

	while(1)                            
	{
		LED3_TOGGLE;
		Usart_SendTick(SysTick_GetTick());
		Delay_ms(500);
	}
}
/*********************************************END OF FILE**********************/
