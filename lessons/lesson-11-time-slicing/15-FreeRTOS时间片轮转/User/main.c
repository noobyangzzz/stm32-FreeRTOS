#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_led.h"
#include "bsp_usart.h"

static void TimeSliceTaskA(void *argument);
static void TimeSliceTaskB(void *argument);
static void SendTaskLog(const char *task_name, const char *message);
static void SendUint32(uint32_t value);

int main(void)
{
  LED_GPIO_Config();
  LED_RGBOFF;

  USART_Config();
  Usart_SendString(DEBUG_USARTx, "lesson-11-time-slicing start\r\n");

  xTaskCreate(TimeSliceTaskA, "task_a", 160, NULL, 2, NULL);
  xTaskCreate(TimeSliceTaskB, "task_b", 160, NULL, 2, NULL);

  vTaskStartScheduler();

  while (1)
  {
  }
}

static void TimeSliceTaskA(void *argument)
{
  TickType_t next_log_tick = 0;

  (void)argument;

  while (1)
  {
    if (xTaskGetTickCount() >= next_log_tick)
    {
      LED3_TOGGLE;
      SendTaskLog("TaskA", "same priority, still ready");
      next_log_tick = xTaskGetTickCount() + pdMS_TO_TICKS(1000);
    }
  }
}

static void TimeSliceTaskB(void *argument)
{
  TickType_t next_log_tick = 0;

  (void)argument;

  while (1)
  {
    if (xTaskGetTickCount() >= next_log_tick)
    {
      SendTaskLog("TaskB", "same priority, still ready");
      next_log_tick = xTaskGetTickCount() + pdMS_TO_TICKS(1000);
    }
  }
}

static void SendTaskLog(const char *task_name, const char *message)
{
  TickType_t tick = xTaskGetTickCount();

  vTaskSuspendAll();
  Usart_SendString(DEBUG_USARTx, "[tick ");
  SendUint32((uint32_t)tick);
  Usart_SendString(DEBUG_USARTx, "] ");
  Usart_SendString(DEBUG_USARTx, (char *)task_name);
  Usart_SendString(DEBUG_USARTx, ": ");
  Usart_SendString(DEBUG_USARTx, (char *)message);
  Usart_SendString(DEBUG_USARTx, "\r\n");
  (void)xTaskResumeAll();
}

static void SendUint32(uint32_t value)
{
  char digits[10];
  uint8_t index = 0;

  if (value == 0)
  {
    Usart_SendByte(DEBUG_USARTx, '0');
    return;
  }

  while (value > 0)
  {
    digits[index++] = (char)('0' + (value % 10));
    value /= 10;
  }

  while (index > 0)
  {
    Usart_SendByte(DEBUG_USARTx, digits[--index]);
  }
}
