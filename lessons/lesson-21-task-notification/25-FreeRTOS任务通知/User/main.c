#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_led.h"
#include "bsp_usart.h"

static void NotifyTask(void *argument);
static void WaitTask(void *argument);
static void CreateTasks(void);
static void CheckCreateResult(BaseType_t result);
static void SendTaskLog(const char *task_name, const char *message);
static void SendUint32(uint32_t value);

static TaskHandle_t wait_task_handle = NULL;

int main(void)
{
  LED_GPIO_Config();
  LED_RGBOFF;

  USART_Config();
  Usart_SendString(DEBUG_USARTx, "lesson-21-task-notification start\r\n");

  CreateTasks();

  vTaskStartScheduler();

  while (1)
  {
  }
}

static void CreateTasks(void)
{
  BaseType_t result;

  result = xTaskCreate(WaitTask,
                       "wait",
                       160,
                       NULL,
                       3,
                       &wait_task_handle);
  CheckCreateResult(result);

  result = xTaskCreate(NotifyTask,
                       "notify",
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

static void NotifyTask(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
    SendTaskLog("NotifyTask", "give notification");
    xTaskNotifyGive(wait_task_handle);
  }
}

static void WaitTask(void *argument)
{
  uint32_t notification_value;

  (void)argument;

  while (1)
  {
    notification_value = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    (void)notification_value;

    LED3_TOGGLE;
    SendTaskLog("WaitTask", "take notification");
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
