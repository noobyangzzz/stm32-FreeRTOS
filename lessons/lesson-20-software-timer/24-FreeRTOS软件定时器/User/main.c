#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "bsp_led.h"
#include "bsp_usart.h"

static void TimerCallback(TimerHandle_t timer);
static void CheckTimerCreated(TimerHandle_t timer);
static void CheckTimerStarted(BaseType_t result);
static void SendTimerLog(const char *timer_name, const char *message);
static void SendUint32(uint32_t value);

#define SOFTWARE_TIMER_PERIOD_MS 1000

static TimerHandle_t periodic_timer = NULL;

int main(void)
{
  LED_GPIO_Config();
  LED_RGBOFF;

  USART_Config();
  Usart_SendString(DEBUG_USARTx, "lesson-20-software-timer start\r\n");

  periodic_timer = xTimerCreate("periodic",
                                pdMS_TO_TICKS(SOFTWARE_TIMER_PERIOD_MS),
                                pdTRUE,
                                NULL,
                                TimerCallback);
  CheckTimerCreated(periodic_timer);

  CheckTimerStarted(xTimerStart(periodic_timer, 0));

  vTaskStartScheduler();

  while (1)
  {
  }
}

static void CheckTimerCreated(TimerHandle_t timer)
{
  if (timer == NULL)
  {
    Usart_SendString(DEBUG_USARTx, "xTimerCreate failed\r\n");
    while (1)
    {
    }
  }
}

static void CheckTimerStarted(BaseType_t result)
{
  if (result != pdPASS)
  {
    Usart_SendString(DEBUG_USARTx, "xTimerStart failed\r\n");
    while (1)
    {
    }
  }
}

static void TimerCallback(TimerHandle_t timer)
{
  (void)timer;

  LED3_TOGGLE;
  SendTimerLog("TimerCallback", "periodic timer expired");
}

static void SendTimerLog(const char *timer_name, const char *message)
{
  TickType_t tick = xTaskGetTickCount();

  vTaskSuspendAll();
  Usart_SendString(DEBUG_USARTx, "[tick ");
  SendUint32((uint32_t)tick);
  Usart_SendString(DEBUG_USARTx, "] ");
  Usart_SendString(DEBUG_USARTx, (char *)timer_name);
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
