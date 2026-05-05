/**
  ******************************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2013-xx-xx
  * @brief   串口中断接收测试
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
#include "bsp_usart.h"

#define LED_BLUE_GPIO_CLK   RCC_APB2Periph_GPIOB
#define LED_BLUE_GPIO_PORT  GPIOB
#define LED_BLUE_GPIO_PIN   GPIO_Pin_1

static void Delay(__IO uint32_t nCount)
{
	for (; nCount != 0; nCount--)
	{
	}
}

static void LED_Blue_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(LED_BLUE_GPIO_CLK, ENABLE);

	GPIO_InitStructure.GPIO_Pin = LED_BLUE_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LED_BLUE_GPIO_PORT, &GPIO_InitStructure);

	GPIO_SetBits(LED_BLUE_GPIO_PORT, LED_BLUE_GPIO_PIN);
}

static void LED_Blue_Toggle(void)
{
	LED_BLUE_GPIO_PORT->ODR ^= LED_BLUE_GPIO_PIN;
}

/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */
int main(void)
{	
  /*初始化蓝色LED，用于确认固件正在运行*/
  LED_Blue_Config();

  /*初始化USART 配置模式为 115200 8-N-1，中断接收*/
  USART_Config();
	
	/* 发送一个字符串 */
	Usart_SendString(DEBUG_USARTx, "USART OK\r\n");
	
  while(1)
	{	
		LED_Blue_Toggle();
		Usart_SendString(DEBUG_USARTx, "USART OK\r\n");
		Delay(0x4FFFFF);
	}	
}
/*********************************************END OF FILE**********************/
