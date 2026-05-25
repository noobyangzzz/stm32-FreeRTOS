#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_led.h"
#include "bsp_usart.h"

static void WorkerTask(void *argument);
static void MonitorTask(void *argument);
static void CreateTasks(void);
static void CheckCreateResult(BaseType_t result);
static void ConsumeHeapStep(void);
static void SendTaskLog(const char *task_name, const char *message);
static void SendMemoryLog(void);
static void SendUint32(uint32_t value);

#define HEAP_CONSUME_STEP_BYTES 2600
#define HEAP_CONSUME_MAX_STEPS 5

static TaskHandle_t worker_task_handle = NULL;
static TaskHandle_t monitor_task_handle = NULL;
static void *heap_blocks[HEAP_CONSUME_MAX_STEPS];
static uint8_t heap_block_count = 0;

int main(void)
{
  LED_GPIO_Config();
  LED_RGBOFF;

  USART_Config();
  Usart_SendString(DEBUG_USARTx, "lesson-23-memory-stack start\r\n");

  CreateTasks();

  vTaskStartScheduler();

  while (1)
  {
  }
}

static void CreateTasks(void)
{
  BaseType_t result;

  result = xTaskCreate(WorkerTask,
                       "worker",
                       180,
                       NULL,
                       1,
                       &worker_task_handle);
  CheckCreateResult(result);

  result = xTaskCreate(MonitorTask,
                       "monitor",
                       220,
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

static void WorkerTask(void *argument)
{
  uint16_t index;
  volatile uint8_t local_buffer[256];

  (void)argument;

  while (1)
  {
    for (index = 0; index < sizeof(local_buffer); index++)
    {
      local_buffer[index] = index;
    }

    LED3_TOGGLE;
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

static void MonitorTask(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(2000));
    SendTaskLog("MonitorTask", "memory check");
    ConsumeHeapStep();
    SendMemoryLog();
  }
}

static void ConsumeHeapStep(void)
{
  void *block;

  if (heap_block_count >= HEAP_CONSUME_MAX_STEPS)
  {
    return;
  }

  block = pvPortMalloc(HEAP_CONSUME_STEP_BYTES);

  if (block == NULL)
  {
    Usart_SendString(DEBUG_USARTx, "  pvPortMalloc returned NULL\r\n");
    return;
  }

  heap_blocks[heap_block_count] = block;
  heap_block_count++;
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

static void SendMemoryLog(void)
{
  size_t heap_free = xPortGetFreeHeapSize();
  size_t heap_min = xPortGetMinimumEverFreeHeapSize();
  UBaseType_t worker_stack_free = uxTaskGetStackHighWaterMark(worker_task_handle);
  UBaseType_t monitor_stack_free = uxTaskGetStackHighWaterMark(monitor_task_handle);

  vTaskSuspendAll();
  Usart_SendString(DEBUG_USARTx, "  heap_blocks=");
  SendUint32((uint32_t)heap_block_count);
  Usart_SendString(DEBUG_USARTx, ", step_bytes=");
  SendUint32((uint32_t)HEAP_CONSUME_STEP_BYTES);
  Usart_SendString(DEBUG_USARTx, "\r\n");

  Usart_SendString(DEBUG_USARTx, "  heap_free=");
  SendUint32((uint32_t)heap_free);
  Usart_SendString(DEBUG_USARTx, ", heap_min=");
  SendUint32((uint32_t)heap_min);
  Usart_SendString(DEBUG_USARTx, "\r\n");

  Usart_SendString(DEBUG_USARTx, "  worker_stack_free_words=");
  SendUint32((uint32_t)worker_stack_free);
  Usart_SendString(DEBUG_USARTx, ", monitor_stack_free_words=");
  SendUint32((uint32_t)monitor_stack_free);
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
