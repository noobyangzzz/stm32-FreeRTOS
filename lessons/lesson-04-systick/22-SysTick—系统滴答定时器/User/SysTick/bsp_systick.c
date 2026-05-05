#include "bsp_systick.h"

static __IO uint32_t s_tick_ms = 0;

void SysTick_Init(void)
{
  SystemCoreClockUpdate();
  SysTick_Config(SystemCoreClock / 1000);
}

void SysTick_IncTick(void)
{
  s_tick_ms++;
}

uint32_t SysTick_GetTick(void)
{
  return s_tick_ms;
}

void Delay_ms(uint32_t ms)
{
  uint32_t start = SysTick_GetTick();

  while ((SysTick_GetTick() - start) < ms)
  {
  }
}
