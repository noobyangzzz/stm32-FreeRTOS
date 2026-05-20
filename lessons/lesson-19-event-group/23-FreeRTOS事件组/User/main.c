#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "bsp_led.h"
#include "bsp_usart.h"

static void EventTaskA(void *argument);
static void EventTaskB(void *argument);
static void WaitTask(void *argument);
static void CreateTasks(void);
static void CheckCreateResult(BaseType_t result);
static void CheckEventGroupCreated(EventGroupHandle_t event_group);
static void SendTaskLog(const char *task_name, const char *message);
static void SendEventBitsLog(const char *task_name, const char *message, EventBits_t bits);
static void SendUint32(uint32_t value);

#define EVENT_BIT_A ( ( EventBits_t ) 0x01 )
#define EVENT_BIT_B ( ( EventBits_t ) 0x02 )

static EventGroupHandle_t sync_event_group = NULL;

int main(void)
{
  LED_GPIO_Config();
  LED_RGBOFF;

  USART_Config();
  Usart_SendString(DEBUG_USARTx, "lesson-19-event-group start\r\n");

  sync_event_group = xEventGroupCreate();
  CheckEventGroupCreated(sync_event_group);

  CreateTasks();

  vTaskStartScheduler();

  while (1)
  {
  }
}

static void CreateTasks(void)
{
  BaseType_t result;

  result = xTaskCreate(EventTaskA,
                       "event_a",
                       160,
                       NULL,
                       2,
                       NULL);
  CheckCreateResult(result);

  result = xTaskCreate(EventTaskB,
                       "event_b",
                       160,
                       NULL,
                       2,
                       NULL);
  CheckCreateResult(result);

  result = xTaskCreate(WaitTask,
                       "wait",
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

static void CheckEventGroupCreated(EventGroupHandle_t event_group)
{
  if (event_group == NULL)
  {
    Usart_SendString(DEBUG_USARTx, "xEventGroupCreate failed\r\n");
    while (1)
    {
    }
  }
}

static void EventTaskA(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
    SendTaskLog("EventTaskA", "set bit A");
    xEventGroupSetBits(sync_event_group, EVENT_BIT_A);
  }
}

static void EventTaskB(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1500));
    SendTaskLog("EventTaskB", "set bit B");
    xEventGroupSetBits(sync_event_group, EVENT_BIT_B);
  }
}

static void WaitTask(void *argument)
{
  EventBits_t bits;

  (void)argument;

  while (1)
  {
    bits = xEventGroupWaitBits(sync_event_group,
                               EVENT_BIT_A | EVENT_BIT_B,
                               pdTRUE,
                               pdTRUE,
                               portMAX_DELAY);

    LED3_TOGGLE;
    SendEventBitsLog("WaitTask", "bit A and bit B ready", bits);
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

static void SendEventBitsLog(const char *task_name, const char *message, EventBits_t bits)
{
  TickType_t tick = xTaskGetTickCount();

  vTaskSuspendAll();
  Usart_SendString(DEBUG_USARTx, "[tick ");
  SendUint32((uint32_t)tick);
  Usart_SendString(DEBUG_USARTx, "] ");
  Usart_SendString(DEBUG_USARTx, (char *)task_name);
  Usart_SendString(DEBUG_USARTx, ": ");
  Usart_SendString(DEBUG_USARTx, (char *)message);
  Usart_SendString(DEBUG_USARTx, ", bits=0x");
  SendUint32((uint32_t)bits);
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
