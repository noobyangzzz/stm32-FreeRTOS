#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "bsp_led.h"
#include "bsp_usart.h"

static void LowTask(void *argument);
static void HighTask(void *argument);
static void CreateTasks(void);
static void CheckCreateResult(BaseType_t result);
static void CheckMutexCreated(SemaphoreHandle_t mutex);
static void SendTaskLog(const char *task_name, const char *message);
static void SendUint32(uint32_t value);

static SemaphoreHandle_t shared_resource_mutex = NULL;

int main(void)
{
  LED_GPIO_Config();
  LED_RGBOFF;

  USART_Config();
  Usart_SendString(DEBUG_USARTx, "lesson-18-mutex start\r\n");

  shared_resource_mutex = xSemaphoreCreateMutex();
  CheckMutexCreated(shared_resource_mutex);

  CreateTasks();

  vTaskStartScheduler();

  while (1)
  {
  }
}

static void CreateTasks(void)
{
  BaseType_t result;

  result = xTaskCreate(LowTask,
                       "low",
                       160,
                       NULL,
                       1,
                       NULL);
  CheckCreateResult(result);

  result = xTaskCreate(HighTask,
                       "high",
                       160,
                       NULL,
                       3,
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

static void CheckMutexCreated(SemaphoreHandle_t mutex)
{
  if (mutex == NULL)
  {
    Usart_SendString(DEBUG_USARTx, "xSemaphoreCreateMutex failed\r\n");
    while (1)
    {
    }
  }
}

static void LowTask(void *argument)
{
  (void)argument;

  while (1)
  {
    SendTaskLog("LowTask", "try take mutex");

    if (xSemaphoreTake(shared_resource_mutex, portMAX_DELAY) == pdPASS)
    {
      SendTaskLog("LowTask", "take mutex, use shared resource");
      vTaskDelay(pdMS_TO_TICKS(3000));
      SendTaskLog("LowTask", "give mutex");
      xSemaphoreGive(shared_resource_mutex);
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

static void HighTask(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
    SendTaskLog("HighTask", "try take mutex");

    if (xSemaphoreTake(shared_resource_mutex, portMAX_DELAY) == pdPASS)
    {
      LED3_TOGGLE;
      SendTaskLog("HighTask", "take mutex, use shared resource");
      vTaskDelay(pdMS_TO_TICKS(500));
      SendTaskLog("HighTask", "give mutex");
      xSemaphoreGive(shared_resource_mutex);
    }

    vTaskDelay(pdMS_TO_TICKS(3000));
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
