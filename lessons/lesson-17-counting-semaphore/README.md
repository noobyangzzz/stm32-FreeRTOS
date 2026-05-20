# Lesson 17：计数信号量 Counting Semaphore

## 本课目标

上一课讲二值信号量：

```text
最大值固定为 1，只表示有没有事件。
```

这一课讲计数信号量：

```text
最大值可以设置成 N，可以累计多个事件或资源数量。
```

本课使用：

```c
event_semaphore = xSemaphoreCreateCounting(5, 0);
```

含义：

```text
最大计数值：5
初始计数值：0
```

## 对比上一个 lesson

对比 Lesson 16，本课仍然使用 `SignalTask` 和 `WaitTask`。

本课变化：

```text
User/FreeRTOSConfig.h 将 configUSE_COUNTING_SEMAPHORES 从 0 改为 1。
User/main.c 将 xSemaphoreCreateBinary() 改为 xSemaphoreCreateCounting(5, 0)。
SignalTask 每 500ms give 一次，模拟事件快速产生。
WaitTask take 成功后处理 1500ms，模拟处理速度较慢。
日志中新增 count，用来观察当前计数。
Makefile 目标名改为 lesson-17-counting-semaphore。
```

这部分实现的功能：

```text
SignalTask：更快地产生事件，让计数信号量逐步累积。
WaitTask：较慢地消费事件，让计数变化更明显。
count：显示当前信号量计数。
```

## FreeRTOSConfig.h 关键配置

要使用计数信号量，需要打开：

```c
#define configUSE_COUNTING_SEMAPHORES 1
```

否则 `xSemaphoreCreateCounting()` 不可用。

## xSemaphoreCreateCounting()

代码：

```c
event_semaphore = xSemaphoreCreateCounting(5, 0);
CheckSemaphoreCreated(event_semaphore);
```

横向理解：

```text
创建计数信号量 -> 最大计数值 5 -> 初始计数值 0 -> 返回信号量句柄 -> 检查是否创建成功
```

状态变化可以理解为：

```text
give：计数 +1，但不能超过最大值 5。
take：计数 -1，如果当前计数为 0，就可以阻塞等待。
```

## SignalTask

完整函数：

```c
static void SignalTask(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(500));

    if (xSemaphoreGive(event_semaphore) == pdPASS)
    {
      SendTaskCountLog("SignalTask", "give counting semaphore");
    }
    else
    {
      SendTaskCountLog("SignalTask", "counting semaphore full");
    }
  }
}
```

带注释理解：

```c
static void SignalTask(void *argument)
{
  // 当前任务没有使用 xTaskCreate() 第 4 个参数。
  (void)argument;

  while (1)
  {
    // 每 500ms 产生一次事件。
    // 生产速度故意比 WaitTask 的消费速度更快。
    vTaskDelay(pdMS_TO_TICKS(500));

    // give 计数信号量，尝试让计数 +1。
    // 只要当前计数小于最大值 5，就会成功。
    if (xSemaphoreGive(event_semaphore) == pdPASS)
    {
      SendTaskCountLog("SignalTask", "give counting semaphore");
    }
    else
    {
      // 如果当前计数已经等于最大值 5，再 give 就会失败。
      SendTaskCountLog("SignalTask", "counting semaphore full");
    }
  }
}
```

## WaitTask

完整函数：

```c
static void WaitTask(void *argument)
{
  (void)argument;

  while (1)
  {
    if (xSemaphoreTake(event_semaphore, portMAX_DELAY) == pdPASS)
    {
      LED3_TOGGLE;
      SendTaskCountLog("WaitTask", "take counting semaphore");
      vTaskDelay(pdMS_TO_TICKS(1500));
    }
  }
}
```

带注释理解：

```c
static void WaitTask(void *argument)
{
  // 当前任务没有使用 xTaskCreate() 第 4 个参数。
  (void)argument;

  while (1)
  {
    // take 计数信号量，尝试让计数 -1。
    // 如果当前计数为 0，就一直阻塞等待。
    if (xSemaphoreTake(event_semaphore, portMAX_DELAY) == pdPASS)
    {
      // take 成功表示取到了一个事件。
      LED3_TOGGLE;
      SendTaskCountLog("WaitTask", "take counting semaphore");

      // 模拟处理事件需要 1500ms。
      // 因为 SignalTask 每 500ms give 一次，所以处理速度比生产速度慢。
      vTaskDelay(pdMS_TO_TICKS(1500));
    }
  }
}
```

## SendTaskCountLog()

本课新增日志函数：

```c
static void SendTaskCountLog(const char *task_name, const char *message)
{
  TickType_t tick = xTaskGetTickCount();
  UBaseType_t count = uxSemaphoreGetCount(event_semaphore);

  vTaskSuspendAll();
  Usart_SendString(DEBUG_USARTx, "[tick ");
  SendUint32((uint32_t)tick);
  Usart_SendString(DEBUG_USARTx, "] ");
  Usart_SendString(DEBUG_USARTx, (char *)task_name);
  Usart_SendString(DEBUG_USARTx, ": ");
  Usart_SendString(DEBUG_USARTx, (char *)message);
  Usart_SendString(DEBUG_USARTx, ", count=");
  SendUint32((uint32_t)count);
  Usart_SendString(DEBUG_USARTx, "\r\n");
  (void)xTaskResumeAll();
}
```

`uxSemaphoreGetCount()` 用来读取当前计数，便于观察：

```text
give 后 count 增加。
take 后 count 减少。
count 到 5 后，再 give 会失败。
```

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印 lesson-17-counting-semaphore start -> xSemaphoreCreateCounting(5, 0) -> CheckSemaphoreCreated() -> CreateTasks() -> vTaskStartScheduler()
```

`CreateTasks()` 的横向流程：

```text
创建 SignalTask -> 检查创建结果 -> 创建 WaitTask -> 检查创建结果
```

## 预期串口现象

Reset 后先输出：

```text
lesson-17-counting-semaphore start
```

随后可以看到类似：

```text
[tick 500] SignalTask: give counting semaphore, count=1
[tick 503] WaitTask: take counting semaphore, count=0
[tick 1006] SignalTask: give counting semaphore, count=1
[tick 1512] SignalTask: give counting semaphore, count=2
[tick 2006] WaitTask: take counting semaphore, count=1
[tick 2018] SignalTask: give counting semaphore, count=2
```

运行一段时间后，因为生产速度快于消费速度，会看到 count 逐步接近 5。到达 5 后，可能看到：

```text
SignalTask: counting semaphore full, count=5
```

重点观察：

```text
二值信号量最多只能记住 1 个事件。
计数信号量可以累计多个事件。
计数到最大值后，再 give 会失败。
```

## 本课结论

这一课要建立四个判断：

```text
1. 计数信号量可以累计多个事件或资源数量。
2. xSemaphoreGive() 会让计数 +1，但不能超过最大值。
3. xSemaphoreTake() 会让计数 -1；如果计数为 0，任务可以阻塞等待。
4. 当生产速度快于消费速度时，计数会逐步增加，直到达到上限。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd '/home/yang/stm32-FreeRTOS/lessons/lesson-17-counting-semaphore/21-FreeRTOS计数信号量'
make clean
make
```

生成产物：

```text
build/lesson-17-counting-semaphore.elf
build/lesson-17-counting-semaphore.hex
build/lesson-17-counting-semaphore.bin
build/lesson-17-counting-semaphore.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
