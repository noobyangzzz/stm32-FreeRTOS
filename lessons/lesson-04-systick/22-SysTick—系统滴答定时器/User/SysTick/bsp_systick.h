#ifndef __BSP_SYSTICK_H
#define __BSP_SYSTICK_H

#include "stm32f10x.h"

void SysTick_Init(void);
void SysTick_IncTick(void);
uint32_t SysTick_GetTick(void);
void Delay_ms(uint32_t ms);

#endif /* __BSP_SYSTICK_H */
