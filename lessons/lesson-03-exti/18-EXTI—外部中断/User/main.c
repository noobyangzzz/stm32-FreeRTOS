/**
  ******************************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2013-xx-xx
  * @brief   按键测试（中断模式/EXTI模式）
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
#include "bsp_exti.h" 
#include "bsp_usart.h"


/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */ 
int main(void)
{
	/* LED 端口初始化 */
	LED_GPIO_Config();

	/* USART1 初始化，用于观察中断事件 */
	USART_Config();
	Usart_SendString(DEBUG_USARTx, "lesson-03-exti start\r\n");
	  	
	/* 初始化EXTI中断，按下按键会触发中断，
  *  触发中断会进入stm32f10x_it.c文件中的函数
	*  KEY1_IRQHandler和KEY2_IRQHandler，处理中断，反转LED灯并打印日志。
	*/
	EXTI_Key_Config(); 
	
	/* 等待中断，由于使用中断方式，CPU不用轮询按键 */
	while(1)                            
	{
	}
}
/*********************************************END OF FILE**********************/
