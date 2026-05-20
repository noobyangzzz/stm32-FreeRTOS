#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "bsp_led.h"
#include "bsp_usart.h"

typedef struct
{
  uint32_t sequence;
  uint32_t producer_tick;
} QueueMessage_t;

static void ProducerTask(void *argument);
static void ConsumerTask(void *argument);
static void CreateTasks(void);
static void CheckCreateResult(BaseType_t result);
static void CheckQueueCreated(QueueHandle_t queue);
static void SendTaskLog(const char *task_name, const char *message);
static void SendQueueMessageLog(const QueueMessage_t *message);
static void SendUint32(uint32_t value);

static QueueHandle_t message_queue = NULL;

int main(void)
{
  LED_GPIO_Config();
  LED_RGBOFF;

  USART_Config();
  Usart_SendString(DEBUG_USARTx, "lesson-15-queue start\r\n");

  message_queue = xQueueCreate(4, sizeof(QueueMessage_t));
  CheckQueueCreated(message_queue);

  CreateTasks();

  vTaskStartScheduler();

  while (1)
  {
  }
}

static void CreateTasks(void)
{
  BaseType_t result;

  result = xTaskCreate(ProducerTask,
                       "producer",
                       160,
                       NULL,
                       2,
                       NULL);
  CheckCreateResult(result);

  result = xTaskCreate(ConsumerTask,
                       "consumer",
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

static void CheckQueueCreated(QueueHandle_t queue)
{
  if (queue == NULL)
  {
    Usart_SendString(DEBUG_USARTx, "xQueueCreate failed\r\n");
    while (1)
    {
    }
  }
}

static void ProducerTask(void *argument)
{
  QueueMessage_t message;
  uint32_t sequence = 0;

  (void)argument;

  while (1)
  {
    message.sequence = sequence++;
    message.producer_tick = (uint32_t)xTaskGetTickCount();

    if (xQueueSend(message_queue, &message, pdMS_TO_TICKS(100)) == pdPASS)
    {
      LED3_TOGGLE;
      SendTaskLog("ProducerTask", "send message");
    }
    else
    {
      SendTaskLog("ProducerTask", "queue full, send failed");
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

static void ConsumerTask(void *argument)
{
  QueueMessage_t message;

  (void)argument;

  while (1)
  {
    if (xQueueReceive(message_queue, &message, portMAX_DELAY) == pdPASS)
    {
      SendQueueMessageLog(&message);
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

static void SendQueueMessageLog(const QueueMessage_t *message)
{
  TickType_t tick = xTaskGetTickCount();

  vTaskSuspendAll();
  Usart_SendString(DEBUG_USARTx, "[tick ");
  SendUint32((uint32_t)tick);
  Usart_SendString(DEBUG_USARTx, "] ConsumerTask: receive seq=");
  SendUint32(message->sequence);
  Usart_SendString(DEBUG_USARTx, ", producer_tick=");
  SendUint32(message->producer_tick);
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
