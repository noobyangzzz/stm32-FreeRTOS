#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_led.h"
#include "bsp_usart.h"

static void FastTask(void *argument);
static void SlowTask(void *argument);
static void MonitorTask(void *argument);
static void CreateTasks(void);
static void CheckCreateResult(BaseType_t result);
static const char *TaskStateToString(eTaskState state);
static void SendTaskLog(const char *task_name, const char *message);
static void SendRuntimeReport(void);
static void SendTaskStatus(const char *task_name, TaskHandle_t task_handle);
static void SendUint32(uint32_t value);

static TaskHandle_t fast_task_handle = NULL;
static TaskHandle_t slow_task_handle = NULL;
static TaskHandle_t monitor_task_handle = NULL;

int main(void)
{
  LED_GPIO_Config();
  LED_RGBOFF;

  USART_Config();
  Usart_SendString(DEBUG_USARTx, "lesson-24-runtime-debug start\r\n");

  CreateTasks();

  vTaskStartScheduler();

  while (1)
  {
  }
}

static void CreateTasks(void)
{
  BaseType_t result;

  result = xTaskCreate(FastTask,
                       "fast",
                       160,
                       NULL,
                       2,
                       &fast_task_handle);
  CheckCreateResult(result);

  result = xTaskCreate(SlowTask,
                       "slow",
                       160,
                       NULL,
                       1,
                       &slow_task_handle);
  CheckCreateResult(result);

  result = xTaskCreate(MonitorTask,
                       "monitor",
                       220,
                       NULL,
                       3,
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

static void FastTask(void *argument)
{
  (void)argument;

  while (1)
  {
    LED3_TOGGLE;
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

static void SlowTask(void *argument)
{
  (void)argument;

  while (1)
  {
    SendTaskLog("SlowTask", "periodic work");
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

static void MonitorTask(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(2000));
    SendRuntimeReport();
  }
}

void vApplicationMallocFailedHook(void)
{
  Usart_SendString(DEBUG_USARTx, "malloc failed hook\r\n");
  while (1)
  {
  }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
  (void)task;

  Usart_SendString(DEBUG_USARTx, "stack overflow hook: ");
  Usart_SendString(DEBUG_USARTx, task_name);
  Usart_SendString(DEBUG_USARTx, "\r\n");

  while (1)
  {
  }
}

static const char *TaskStateToString(eTaskState state)
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

static void SendRuntimeReport(void)
{
  TickType_t tick = xTaskGetTickCount();

  vTaskSuspendAll();
  Usart_SendString(DEBUG_USARTx, "[tick ");
  SendUint32((uint32_t)tick);
  Usart_SendString(DEBUG_USARTx, "] Runtime report\r\n");

  Usart_SendString(DEBUG_USARTx, "  task_count=");
  SendUint32((uint32_t)uxTaskGetNumberOfTasks());
  Usart_SendString(DEBUG_USARTx, ", heap_free=");
  SendUint32((uint32_t)xPortGetFreeHeapSize());
  Usart_SendString(DEBUG_USARTx, ", heap_min=");
  SendUint32((uint32_t)xPortGetMinimumEverFreeHeapSize());
  Usart_SendString(DEBUG_USARTx, "\r\n");

  SendTaskStatus("fast", fast_task_handle);
  SendTaskStatus("slow", slow_task_handle);
  SendTaskStatus("monitor", monitor_task_handle);
  (void)xTaskResumeAll();
}

static void SendTaskStatus(const char *task_name, TaskHandle_t task_handle)
{
  Usart_SendString(DEBUG_USARTx, "  ");
  Usart_SendString(DEBUG_USARTx, (char *)task_name);
  Usart_SendString(DEBUG_USARTx, ": priority=");
  SendUint32((uint32_t)uxTaskPriorityGet(task_handle));
  Usart_SendString(DEBUG_USARTx, ", state=");
  Usart_SendString(DEBUG_USARTx, (char *)TaskStateToString(eTaskGetState(task_handle)));
  Usart_SendString(DEBUG_USARTx, ", stack_free_words=");
  SendUint32((uint32_t)uxTaskGetStackHighWaterMark(task_handle));
  Usart_SendString(DEBUG_USARTx, "\r\n");
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
