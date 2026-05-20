#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_led.h"
#include "bsp_usart.h"

static void HighPriorityTask(void *argument);
static void MediumLedTask(void *argument);
static void LowPriorityTask(void *argument);
static void SendTaskLog(const char *task_name, const char *message);
static void SendUint32(uint32_t value);

int main(void)
{
  LED_GPIO_Config();
  LED_RGBOFF;

  USART_Config();
  Usart_SendString(DEBUG_USARTx, "lesson-10-task-priority start\r\n");

  xTaskCreate(HighPriorityTask, "high", 160, NULL, 3, NULL);
  xTaskCreate(MediumLedTask, "medium", 160, NULL, 2, NULL);
  xTaskCreate(LowPriorityTask, "low", 160, NULL, 1, NULL);

  vTaskStartScheduler();

  while (1)
  {
  }
}

static void HighPriorityTask(void *argument)
{
  (void)argument;

  while (1)
  {
    SendTaskLog("HighPriorityTask", "run, then block 1000ms");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

static void MediumLedTask(void *argument)
{
  (void)argument;

  while (1)
  {
    LED3_TOGGLE;
    SendTaskLog("MediumLedTask", "toggle LED3, then block 1000ms");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

static void LowPriorityTask(void *argument)
{
  (void)argument;

  while (1)
  {
    SendTaskLog("LowPriorityTask", "run, then block 1000ms");
    vTaskDelay(pdMS_TO_TICKS(1000));
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
