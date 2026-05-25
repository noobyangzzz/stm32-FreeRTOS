#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_exti.h"
#include "bsp_led.h"
#include "bsp_usart.h"

#define KEY1_NOTIFY_BIT ( ( uint32_t ) 0x01 )
#define KEY2_NOTIFY_BIT ( ( uint32_t ) 0x02 )
#define ALL_KEY_NOTIFY_BITS ( KEY1_NOTIFY_BIT | KEY2_NOTIFY_BIT )

static void KeyWaitTask(void *argument);
static void HeartbeatTask(void *argument);
static void CreateTasks(void);
static void CheckCreateResult(BaseType_t result);
static void SendTaskLog(const char *task_name, const char *message);
static void SendKeyLog(uint32_t key_bits);
static void SendUint32(uint32_t value);

static TaskHandle_t key_wait_task_handle = NULL;

int main(void)
{
  LED_GPIO_Config();
  LED_RGBOFF;

  USART_Config();
  Usart_SendString(DEBUG_USARTx, "lesson-22-from-isr-notification start\r\n");

  EXTI_Key_Config();
  CreateTasks();

  vTaskStartScheduler();

  while (1)
  {
  }
}

void NotifyKeyTaskFromISR(uint32_t key_bits)
{
  BaseType_t higher_priority_task_woken = pdFALSE;

  if (key_wait_task_handle != NULL)
  {
    (void)xTaskNotifyFromISR(key_wait_task_handle,
                             key_bits,
                             eSetBits,
                             &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
  }
}

static void CreateTasks(void)
{
  BaseType_t result;

  result = xTaskCreate(KeyWaitTask,
                       "key_wait",
                       180,
                       NULL,
                       3,
                       &key_wait_task_handle);
  CheckCreateResult(result);

  result = xTaskCreate(HeartbeatTask,
                       "heartbeat",
                       160,
                       NULL,
                       1,
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

static void KeyWaitTask(void *argument)
{
  uint32_t key_bits;

  (void)argument;

  while (1)
  {
    key_bits = 0;
    (void)xTaskNotifyWait(0,
                          ALL_KEY_NOTIFY_BITS,
                          &key_bits,
                          portMAX_DELAY);

    if ((key_bits & KEY1_NOTIFY_BIT) != 0)
    {
      LED1_TOGGLE;
    }

    if ((key_bits & KEY2_NOTIFY_BIT) != 0)
    {
      LED2_TOGGLE;
    }

    SendKeyLog(key_bits);
  }
}

static void HeartbeatTask(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(2000));
    LED3_TOGGLE;
    SendTaskLog("HeartbeatTask", "system running");
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

static void SendKeyLog(uint32_t key_bits)
{
  TickType_t tick = xTaskGetTickCount();

  vTaskSuspendAll();
  Usart_SendString(DEBUG_USARTx, "[tick ");
  SendUint32((uint32_t)tick);
  Usart_SendString(DEBUG_USARTx, "] KeyWaitTask: notify bits=0x");
  SendUint32(key_bits);
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
