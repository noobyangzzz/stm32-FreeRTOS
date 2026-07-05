# Lesson 25：FreeRTOS 综合小实验

## 本课目标

这一课不是学习一个新的单独 API，而是把前面学过的能力组合成一个小系统。

系统里同时包含：

- USART 串口调试输出
- LED 心跳任务
- EXTI 按键中断
- 中断到任务的通知
- 软件定时器
- 互斥量保护串口输出
- heap / stack / task state 运行状态监控

这节课的重点是理解一个 FreeRTOS 应用程序的整体组织方式：中断只做很少的事，任务负责主要业务逻辑，调试信息通过串口持续观察。

## 工程路径

```text
lessons/lesson-25-final-project/29-FreeRTOS综合小实验
```

## 编译方式

```bash
cd lessons/lesson-25-final-project/29-FreeRTOS综合小实验
make clean
make
```

生成文件：

```text
build/lesson-25-final-project.elf
build/lesson-25-final-project.hex
build/lesson-25-final-project.bin
build/lesson-25-final-project.map
```

本次编译结果：

```text
text    data     bss     dec     hex
17088     36   16716   33840    8430
```

## 预期现象

Reset 后，串口打印：

```text
lesson-25-final-project start
```

不按键时：

```text
HeartbeatTask 周期翻转蓝灯 LED3。
ControlTask 每 3000 tick 收到软件定时器事件，打印当前模式。
MonitorTask 每 5000 tick 打印任务数量、heap 和各任务 stack 水位。
```

按 K1：

```text
K1 EXTI 中断 -> 通知 ControlTask -> 切换心跳模式
SLOW 模式：蓝灯约 1000ms 翻转一次
FAST 模式：蓝灯约 250ms 翻转一次
```

按 K2：

```text
K2 EXTI 中断 -> 通知 ControlTask -> 打开/关闭软件定时器状态上报
```

## 主流程

```text
main()
-> LED_GPIO_Config()
-> USART_Config()
-> EXTI_Key_Config()
-> CreateKernelObjects()
-> CreateTasks()
-> xTimerStart(status_timer)
-> vTaskStartScheduler()
```

进入调度器后：

```text
HeartbeatTask：按照当前 heartbeat_period_ms 翻转 LED3
ControlTask：等待按键中断或软件定时器通知
MonitorTask：周期打印系统运行状态
Timer Service Task：执行 StatusTimerCallback()
```

## 新增内容对比上一课

对比 Lesson 24，User 文件夹里的核心变化是：

```text
User/main.c
-> 从单纯运行状态监控，变成综合应用主逻辑
-> 新增 ControlTask、HeartbeatTask、MonitorTask
-> 新增 StatusTimerCallback
-> 新增 NotifyKeyTaskFromISR
-> 新增 uart_mutex 保护串口输出

User/FreeRTOSConfig.h
-> 打开 configUSE_TIMERS
-> 打开 configUSE_MUTEXES
-> 打开 INCLUDE_uxTaskPriorityGet
-> 打开 INCLUDE_eTaskGetState
-> 打开 INCLUDE_uxTaskGetStackHighWaterMark
-> 打开 malloc failed hook 和 stack overflow hook

User/Key/bsp_exti.c
-> 保留 K1/K2 EXTI 配置
-> K1 使用 EXTI0，上升沿触发
-> K2 使用 EXTI15_10，下降沿触发

User/stm32f10x_it.c
-> KEY1_IRQHandler 清除中断标志并通知任务
-> KEY2_IRQHandler 清除中断标志并通知任务

Makefile
-> TARGET 改成 lesson-25-final-project
-> 编译 FreeRTOS/Source/timers.c
```

## main.c 函数级讲解

### main()

```c
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
```

横向理解：

```text
初始化板级外设 -> 创建 RTOS 对象 -> 创建任务 -> 启动软件定时器 -> 启动调度器
```

这里的重点是：`main()` 只负责系统启动，不在 `while(1)` 里写业务逻辑。业务逻辑进入 FreeRTOS 后由任务执行。

### CreateKernelObjects()

```c
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
```

横向理解：

```text
创建串口互斥量 -> 创建周期软件定时器 -> 检查对象是否创建成功
```

`uart_mutex` 的作用是保护串口输出。多个任务同时打印时，如果没有互斥量，日志可能交叉在一起。

`status_timer` 是一个周期软件定时器，每 3000ms 到期一次。它不是硬件定时器中断，而是由 FreeRTOS 的 Timer Service Task 管理。

### CreateTasks()

```c
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
```

任务优先级安排：

```text
ControlTask：优先级 3，处理按键和系统控制，最高
MonitorTask：优先级 2，周期打印系统状态
HeartbeatTask：优先级 1，最低，负责 LED 心跳
```

为什么 `ControlTask` 优先级最高：

```text
按键中断发生后，ISR 只通知任务，不处理复杂业务。
ControlTask 收到通知后应该尽快运行，完成模式切换。
```

这就是“中断短、任务处理”的典型写法。

### NotifyKeyTaskFromISR()

```c
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
```

横向理解：

```text
按键 ISR 调用 NotifyKeyTaskFromISR()
-> xTaskNotifyFromISR() 把 key bit 发给 ControlTask
-> 如果 ControlTask 被唤醒且优先级更高，higher_priority_task_woken 变成 pdTRUE
-> portYIELD_FROM_ISR() 请求中断退出后立刻调度
```

这里没有在中断里打印串口，也没有在中断里切换系统模式。中断只负责“把事件送出去”。

### ControlTask()

```c
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
```

横向理解：

```text
ControlTask 阻塞等待通知
-> 收到 K1 bit：切换 SLOW / FAST 心跳模式
-> 收到 K2 bit：打开 / 关闭状态上报
-> 收到 TIMER bit：打印当前系统控制状态
-> 没有通知时一直阻塞，不占用 CPU
```

`xTaskNotifyWait()` 在这里相当于一个“事件入口”。按键事件和定时器事件都汇聚到 `ControlTask`，由它统一做控制决策。

### HeartbeatTask()

```c
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
```

横向理解：

```text
读取当前心跳周期 -> 阻塞延时 -> 翻转 LED3 -> 打印日志 -> 重复
```

`heartbeat_period_ms` 由 `ControlTask` 修改：

```text
SLOW：1000ms
FAST：250ms
```

所以 K1 并不是直接操作 HeartbeatTask，而是修改它依赖的运行参数。

### StatusTimerCallback()

```c
static void StatusTimerCallback(TimerHandle_t timer)
{
  (void)timer;

  if ((control_task_handle != NULL) && (status_report_enabled == pdTRUE))
  {
    (void)xTaskNotify(control_task_handle, TIMER_NOTIFY_BIT, eSetBits);
  }
}
```

横向理解：

```text
软件定时器到期
-> Timer Service Task 执行回调
-> 回调函数不直接打印复杂内容
-> 只通知 ControlTask：该打印状态了
```

这样做的好处是让软件定时器回调保持很短，复杂业务仍然放在普通任务里。

### MonitorTask()

```c
static void MonitorTask(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(MONITOR_PERIOD_MS));
    SendRuntimeReport();
  }
}
```

横向理解：

```text
每 5000ms 运行一次 -> 查询任务数量、heap、stack、task state -> 串口打印
```

它的作用不是控制业务，而是观察系统是否健康。

### SendRuntimeReport()

这个函数使用了 Lesson 24 学过的几个接口：

```text
uxTaskGetNumberOfTasks()
xPortGetFreeHeapSize()
xPortGetMinimumEverFreeHeapSize()
eTaskGetState()
uxTaskPriorityGet()
uxTaskGetStackHighWaterMark()
```

横向理解：

```text
打印任务数量和 heap
-> 打印 ControlTask 状态
-> 打印 HeartbeatTask 状态
-> 打印 MonitorTask 状态
```

这就是把 Lesson 24 的“调试接口”真正放进一个应用程序里使用。

## 完整事件链路

### K1 模式切换

```text
按下 K1
-> EXTI0 产生中断
-> KEY1_IRQHandler()
-> EXTI_ClearITPendingBit()
-> NotifyKeyTaskFromISR(KEY1_NOTIFY_BIT)
-> xTaskNotifyFromISR(control_task_handle, KEY1_NOTIFY_BIT, eSetBits, ...)
-> portYIELD_FROM_ISR()
-> ControlTask 从 xTaskNotifyWait() 返回
-> 判断 KEY1_NOTIFY_BIT
-> 切换 app_mode 和 heartbeat_period_ms
-> HeartbeatTask 后续按新周期闪烁 LED3
```

### K2 状态上报开关

```text
按下 K2
-> EXTI15_10 产生中断
-> KEY2_IRQHandler()
-> EXTI_ClearITPendingBit()
-> NotifyKeyTaskFromISR(KEY2_NOTIFY_BIT)
-> ControlTask 收到 KEY2_NOTIFY_BIT
-> status_report_enabled 在 ON / OFF 之间切换
-> 软件定时器是否继续触发状态打印由这个变量决定
```

### 软件定时器上报

```text
status_timer 每 3000ms 到期
-> Timer Service Task 调用 StatusTimerCallback()
-> StatusTimerCallback 通知 ControlTask
-> ControlTask 收到 TIMER_NOTIFY_BIT
-> 打印当前 mode、heartbeat_ms、notify_bits
```

### 运行状态监控

```text
MonitorTask 每 5000ms 醒来
-> 查询任务数量
-> 查询 heap 剩余和历史最低 heap
-> 查询 control / heartbeat / monitor 的状态、优先级、栈水位
-> 串口输出 runtime report
```

## 这节课的核心理解

最终综合实验想表达的不是“API 越多越好”，而是 RTOS 应用的分层方式：

```text
中断层：只捕获硬件事件，尽快退出
控制任务：处理系统模式和事件决策
工作任务：执行周期性业务
定时器：产生周期性软件事件
监控任务：观察系统资源和任务状态
互斥量：保护共享串口输出
```

这就是一个小型 FreeRTOS 应用的基本骨架。
