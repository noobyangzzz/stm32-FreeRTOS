#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "bsp_led.h"
#include "bsp_usart.h"

static void SignalTask(void *argument);
static void WaitTask(void *argument);
static void CreateTasks(void);
static void CheckCreateResult(BaseType_t result);
static void CheckSemaphoreCreated(SemaphoreHandle_t semaphore);
static void SendTaskLog(const char *task_name, const char *message);
static void SendUint32(uint32_t value);

static SemaphoreHandle_t event_semaphore = NULL;

int main(void)
{
  LED_GPIO_Config();
  LED_RGBOFF;

  USART_Config();
  Usart_SendString(DEBUG_USARTx, "lesson-16-binary-semaphore start\r\n");

  event_semaphore = xSemaphoreCreateBinary();
  CheckSemaphoreCreated(event_semaphore);

  CreateTasks();

  vTaskStartScheduler();

  while (1)
  {
  }
}

static void CreateTasks(void)
{
  BaseType_t result;

  result = xTaskCreate(SignalTask,
                       "signal",
                       160,
                       NULL,
                       2,
                       NULL);
  CheckCreateResult(result);

  result = xTaskCreate(WaitTask,
                       "wait",
                       160,
                       NULL,
                       2,
                       NULL);
  CheckCreateResult(result);
}

static void CheckCreateResult(BaseType_t result)
{
  if (result != pdPASS)
  {
    Usart_SendString(DEBUG_USARTx, "xTaskCreate failed\r\n");
    while (1)
    {
    }
  }
}

static void CheckSemaphoreCreated(SemaphoreHandle_t semaphore)
{
  if (semaphore == NULL)
  {
    Usart_SendString(DEBUG_USARTx, "xSemaphoreCreateBinary failed\r\n");
    while (1)
    {
    }
  }
}

static void SignalTask(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));

    if (xSemaphoreGive(event_semaphore) == pdPASS)
    {
      SendTaskLog("SignalTask", "give semaphore");
    }
    else
    {
      SendTaskLog("SignalTask", "semaphore already available");
    }
  }
}

static void WaitTask(void *argument)
{
  (void)argument;

  while (1)
  {
    if (xSemaphoreTake(event_semaphore, portMAX_DELAY) == pdPASS)
    {
      LED3_TOGGLE;
      SendTaskLog("WaitTask", "take semaphore");
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
