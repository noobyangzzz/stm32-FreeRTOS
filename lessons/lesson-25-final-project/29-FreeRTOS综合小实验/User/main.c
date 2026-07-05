#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "bsp_exti.h"
#include "bsp_led.h"
#include "bsp_usart.h"

#define KEY1_NOTIFY_BIT ( ( uint32_t ) 0x01 )
#define KEY2_NOTIFY_BIT ( ( uint32_t ) 0x02 )
#define TIMER_NOTIFY_BIT ( ( uint32_t ) 0x04 )
#define ALL_NOTIFY_BITS ( KEY1_NOTIFY_BIT | KEY2_NOTIFY_BIT | TIMER_NOTIFY_BIT )

#define HEARTBEAT_SLOW_MS ( ( uint32_t ) 1000 )
#define HEARTBEAT_FAST_MS ( ( uint32_t ) 250 )
#define STATUS_TIMER_MS   ( ( uint32_t ) 3000 )
#define MONITOR_PERIOD_MS ( ( uint32_t ) 5000 )

typedef enum
{
  APP_MODE_SLOW = 0,
  APP_MODE_FAST
} AppMode_t;

static void ControlTask(void *argument);
static void HeartbeatTask(void *argument);
static void MonitorTask(void *argument);
static void StatusTimerCallback(TimerHandle_t timer);
static void CreateTasks(void);
static void CreateKernelObjects(void);
static void CheckCreateResult(BaseType_t result);
static void CheckPointer(void *pointer, const char *message);
static void SendLogPrefix(const char *task_name);
static void SendLogLine(const char *task_name, const char *message);
static void SendControlEvent(uint32_t notify_bits);
static void SendRuntimeReport(void);
static void SendTaskReport(const char *name, TaskHandle_t handle);
static const char *TaskStateName(eTaskState state);
static void SendUint32Dec(uint32_t value);
static void SendUint32Hex(uint32_t value);
static void UartLock(void);
static void UartUnlock(void);

static TaskHandle_t control_task_handle = NULL;
static TaskHandle_t heartbeat_task_handle = NULL;
static TaskHandle_t monitor_task_handle = NULL;
static SemaphoreHandle_t uart_mutex = NULL;
static TimerHandle_t status_timer = NULL;
static volatile AppMode_t app_mode = APP_MODE_SLOW;
static volatile uint32_t heartbeat_period_ms = HEARTBEAT_SLOW_MS;
static volatile BaseType_t status_report_enabled = pdTRUE;

int main(void)
{
  LED_GPIO_Config();
  LED_RGBOFF;

  USART_Config();
  Usart_SendString(DEBUG_USARTx, "lesson-25-final-project start\r\n");

  EXTI_Key_Config();
  CreateKernelObjects();
  CreateTasks();

  if (xTimerStart(status_timer, 0) != pdPASS)
  {
    Usart_SendString(DEBUG_USARTx, "xTimerStart failed\r\n");
    while (1)
    {
    }
  }

  vTaskStartScheduler();

  while (1)
  {
  }
}

void NotifyKeyTaskFromISR(uint32_t key_bits)
{
  BaseType_t higher_priority_task_woken = pdFALSE;

  if (control_task_handle != NULL)
  {
    (void)xTaskNotifyFromISR(control_task_handle,
                             key_bits,
                             eSetBits,
                             &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
  }
}

void vApplicationMallocFailedHook(void)
{
  Usart_SendString(DEBUG_USARTx, "Malloc failed\r\n");
  taskDISABLE_INTERRUPTS();
  while (1)
  {
  }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
  (void)task;
  Usart_SendString(DEBUG_USARTx, "Stack overflow: ");
  Usart_SendString(DEBUG_USARTx, task_name);
  Usart_SendString(DEBUG_USARTx, "\r\n");
  taskDISABLE_INTERRUPTS();
  while (1)
  {
  }
}

static void CreateKernelObjects(void)
{
  uart_mutex = xSemaphoreCreateMutex();
  CheckPointer(uart_mutex, "xSemaphoreCreateMutex failed");

  status_timer = xTimerCreate("status_timer",
                              pdMS_TO_TICKS(STATUS_TIMER_MS),
                              pdTRUE,
                              NULL,
                              StatusTimerCallback);
  CheckPointer(status_timer, "xTimerCreate failed");
}

static void CreateTasks(void)
{
  BaseType_t result;

  result = xTaskCreate(ControlTask,
                       "control",
                       240,
                       NULL,
                       3,
                       &control_task_handle);
  CheckCreateResult(result);

  result = xTaskCreate(HeartbeatTask,
                       "heartbeat",
                       180,
                       NULL,
                       1,
                       &heartbeat_task_handle);
  CheckCreateResult(result);

  result = xTaskCreate(MonitorTask,
                       "monitor",
                       260,
                       NULL,
                       2,
                       &monitor_task_handle);
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

static void CheckPointer(void *pointer, const char *message)
{
  if (pointer == NULL)
  {
    Usart_SendString(DEBUG_USARTx, (char *)message);
    Usart_SendString(DEBUG_USARTx, "\r\n");
    while (1)
    {
    }
  }
}

static void ControlTask(void *argument)
{
  uint32_t notify_bits;

  (void)argument;

  while (1)
  {
    notify_bits = 0;
    (void)xTaskNotifyWait(0,
                          ALL_NOTIFY_BITS,
                          &notify_bits,
                          portMAX_DELAY);

    if ((notify_bits & KEY1_NOTIFY_BIT) != 0)
    {
      if (app_mode == APP_MODE_SLOW)
      {
        app_mode = APP_MODE_FAST;
        heartbeat_period_ms = HEARTBEAT_FAST_MS;
        LED1_ON;
        SendLogLine("ControlTask", "K1 pressed, mode=FAST");
      }
      else
      {
        app_mode = APP_MODE_SLOW;
        heartbeat_period_ms = HEARTBEAT_SLOW_MS;
        LED1_OFF;
        SendLogLine("ControlTask", "K1 pressed, mode=SLOW");
      }
    }

    if ((notify_bits & KEY2_NOTIFY_BIT) != 0)
    {
      status_report_enabled = (status_report_enabled == pdTRUE) ? pdFALSE : pdTRUE;

      if (status_report_enabled == pdTRUE)
      {
        LED2_ON;
        SendLogLine("ControlTask", "K2 pressed, status report=ON");
      }
      else
      {
        LED2_OFF;
        SendLogLine("ControlTask", "K2 pressed, status report=OFF");
      }
    }

    if ((notify_bits & TIMER_NOTIFY_BIT) != 0)
    {
      SendControlEvent(notify_bits);
    }
  }
}

static void HeartbeatTask(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(heartbeat_period_ms));
    LED3_TOGGLE;
    SendLogLine("HeartbeatTask", "toggle LED3");
  }
}

static void MonitorTask(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(MONITOR_PERIOD_MS));
    SendRuntimeReport();
  }
}

static void StatusTimerCallback(TimerHandle_t timer)
{
  (void)timer;

  if ((control_task_handle != NULL) && (status_report_enabled == pdTRUE))
  {
    (void)xTaskNotify(control_task_handle, TIMER_NOTIFY_BIT, eSetBits);
  }
}

static void SendLogPrefix(const char *task_name)
{
  Usart_SendString(DEBUG_USARTx, "[tick ");
  SendUint32Dec((uint32_t)xTaskGetTickCount());
  Usart_SendString(DEBUG_USARTx, "] ");
  Usart_SendString(DEBUG_USARTx, (char *)task_name);
  Usart_SendString(DEBUG_USARTx, ": ");
}

static void SendLogLine(const char *task_name, const char *message)
{
  UartLock();
  SendLogPrefix(task_name);
  Usart_SendString(DEBUG_USARTx, (char *)message);
  Usart_SendString(DEBUG_USARTx, "\r\n");
  UartUnlock();
}

static void SendControlEvent(uint32_t notify_bits)
{
  UartLock();
  SendLogPrefix("ControlTask");
  Usart_SendString(DEBUG_USARTx, "timer status, mode=");
  Usart_SendString(DEBUG_USARTx, (app_mode == APP_MODE_FAST) ? "FAST" : "SLOW");
  Usart_SendString(DEBUG_USARTx, ", heartbeat_ms=");
  SendUint32Dec(heartbeat_period_ms);
  Usart_SendString(DEBUG_USARTx, ", notify_bits=0x");
  SendUint32Hex(notify_bits);
  Usart_SendString(DEBUG_USARTx, "\r\n");
  UartUnlock();
}

static void SendRuntimeReport(void)
{
  UartLock();
  SendLogPrefix("MonitorTask");
  Usart_SendString(DEBUG_USARTx, "runtime report\r\n");

  Usart_SendString(DEBUG_USARTx, "  tasks=");
  SendUint32Dec((uint32_t)uxTaskGetNumberOfTasks());
  Usart_SendString(DEBUG_USARTx, ", heap_free=");
  SendUint32Dec((uint32_t)xPortGetFreeHeapSize());
  Usart_SendString(DEBUG_USARTx, ", heap_min=");
  SendUint32Dec((uint32_t)xPortGetMinimumEverFreeHeapSize());
  Usart_SendString(DEBUG_USARTx, "\r\n");

  SendTaskReport("control", control_task_handle);
  SendTaskReport("heartbeat", heartbeat_task_handle);
  SendTaskReport("monitor", monitor_task_handle);
  UartUnlock();
}

static void SendTaskReport(const char *name, TaskHandle_t handle)
{
  Usart_SendString(DEBUG_USARTx, "  ");
  Usart_SendString(DEBUG_USARTx, (char *)name);
  Usart_SendString(DEBUG_USARTx, ": state=");
  Usart_SendString(DEBUG_USARTx, (char *)TaskStateName(eTaskGetState(handle)));
  Usart_SendString(DEBUG_USARTx, ", prio=");
  SendUint32Dec((uint32_t)uxTaskPriorityGet(handle));
  Usart_SendString(DEBUG_USARTx, ", stack_free_words=");
  SendUint32Dec((uint32_t)uxTaskGetStackHighWaterMark(handle));
  Usart_SendString(DEBUG_USARTx, "\r\n");
}

static const char *TaskStateName(eTaskState state)
{
  switch (state)
  {
    case eRunning:
      return "Running";
    case eReady:
      return "Ready";
    case eBlocked:
      return "Blocked";
    case eSuspended:
      return "Suspended";
    case eDeleted:
      return "Deleted";
    default:
      return "Invalid";
  }
}

static void SendUint32Dec(uint32_t value)
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

static void SendUint32Hex(uint32_t value)
{
  char digits[8];
  uint8_t index = 0;
  uint8_t nibble;

  if (value == 0)
  {
    Usart_SendByte(DEBUG_USARTx, '0');
    return;
  }

  while (value > 0)
  {
    nibble = (uint8_t)(value & 0x0F);
    digits[index++] = (char)((nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10));
    value >>= 4;
  }

  while (index > 0)
  {
    Usart_SendByte(DEBUG_USARTx, digits[--index]);
  }
}

static void UartLock(void)
{
  if (uart_mutex != NULL)
  {
    (void)xSemaphoreTake(uart_mutex, portMAX_DELAY);
  }
}

static void UartUnlock(void)
{
  if (uart_mutex != NULL)
  {
    (void)xSemaphoreGive(uart_mutex);
  }
}
