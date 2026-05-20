#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_led.h"
#include "bsp_usart.h"

typedef struct
{
  const char *name;
  const char *message;
  TickType_t period_ticks;
  uint8_t toggle_led;
} TaskConfig_t;

static void ParamTask(void *argument);
static void ControlTask(void *argument);
static void CreateTasks(void);
static void CheckCreateResult(BaseType_t result);
static void SendTaskLog(const char *task_name, const char *message);
static void SendUint32(uint32_t value);

static const TaskConfig_t task_a_config =
{
  "TaskA",
  "parameter period 500ms",
  pdMS_TO_TICKS(500),
  1
};

static const TaskConfig_t task_b_config =
{
  "TaskB",
  "parameter period 1000ms",
  pdMS_TO_TICKS(1000),
  0
};

static TaskHandle_t task_a_handle = NULL;
static TaskHandle_t task_b_handle = NULL;

int main(void)
{
  LED_GPIO_Config();
  LED_RGBOFF;

  USART_Config();
  Usart_SendString(DEBUG_USARTx, "lesson-13-task-suspend-resume start\r\n");

  CreateTasks();

  vTaskStartScheduler();

  while (1)
  {
  }
}

static void CreateTasks(void)
{
  BaseType_t result;

  result = xTaskCreate(ParamTask,
                       "task_a",
                       160,
                       (void *)&task_a_config,
                       2,
                       &task_a_handle);
  CheckCreateResult(result);

  result = xTaskCreate(ParamTask,
                       "task_b",
                       160,
                       (void *)&task_b_config,
                       2,
                       &task_b_handle);
  CheckCreateResult(result);

  result = xTaskCreate(ControlTask,
                       "control",
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

static void ParamTask(void *argument)
{
  const TaskConfig_t *config = (const TaskConfig_t *)argument;

  while (1)
  {
    if (config->toggle_led != 0)
    {
      LED3_TOGGLE;
    }

    SendTaskLog(config->name, config->message);
    vTaskDelay(config->period_ticks);
  }
}

static void ControlTask(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(3000));
    SendTaskLog("ControlTask", "suspend TaskA");
    vTaskSuspend(task_a_handle);

    vTaskDelay(pdMS_TO_TICKS(3000));
    SendTaskLog("ControlTask", "resume TaskA");
    vTaskResume(task_a_handle);
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
