# Lesson 23：FreeRTOS 内存与任务栈

## 本课目标

前面的课主要关注任务调度和任务间通信。

这一课开始关注一个更工程化的问题：

```text
FreeRTOS 里的任务、队列、信号量、事件组等对象，它们的内存从哪里来？
每个任务的栈够不够用？
系统还剩多少 heap？
```

本课目标是观察：

```text
MonitorTask 每 2000ms 申请一块 heap，并打印一次 heap 和 stack 信息。
WorkerTask 每 500ms 翻转 LED3，并故意使用一块较大的局部数组消耗任务栈。
串口日志显示 heap_free、heap_min、worker_stack_free_words、monitor_stack_free_words。
```

## 对比上一个 lesson

对比 Lesson 22，本课不再使用 EXTI 中断和 FromISR API。

本课变化：

```text
User/main.c 新增 WorkerTask()。
User/main.c 新增 MonitorTask()。
User/main.c 新增 ConsumeHeapStep()。
User/main.c 新增 SendMemoryLog()。
User/main.c 新增 vApplicationMallocFailedHook()。
User/main.c 新增 vApplicationStackOverflowHook()。
User/FreeRTOSConfig.h 打开 INCLUDE_uxTaskGetStackHighWaterMark。
User/FreeRTOSConfig.h 打开 configUSE_MALLOC_FAILED_HOOK。
User/FreeRTOSConfig.h 打开 configCHECK_FOR_STACK_OVERFLOW。
Makefile 目标名改为 lesson-23-memory-stack。
```

## heap 和 stack 的区别

在 FreeRTOS 里要区分两种内存：

```text
heap：FreeRTOS 动态创建对象时使用的内存池。
stack：每个任务自己运行函数、保存局部变量、函数调用现场用的栈。
```

`xTaskCreate()` 会从 FreeRTOS heap 里分配：

```text
任务控制块 TCB
任务栈 stack
```

所以任务创建越多、任务栈设得越大，heap 剩余空间就越少。

## FreeRTOSConfig.h 关键配置

本课使用：

```c
#define configTOTAL_HEAP_SIZE ( ( size_t ) ( 16 * 1024 ) )
```

表示 FreeRTOS heap 总大小是：

```text
16KB
```

本课还打开：

```c
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define configUSE_MALLOC_FAILED_HOOK        1
#define configCHECK_FOR_STACK_OVERFLOW      2
```

含义：

```text
INCLUDE_uxTaskGetStackHighWaterMark：允许调用 uxTaskGetStackHighWaterMark() 查询任务栈水位。
configUSE_MALLOC_FAILED_HOOK：动态内存分配失败时调用 vApplicationMallocFailedHook()。
configCHECK_FOR_STACK_OVERFLOW：让 FreeRTOS 检查任务栈溢出。
```

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印 lesson-23-memory-stack start -> CreateTasks() -> vTaskStartScheduler()
```

`CreateTasks()` 的横向流程：

```text
创建 WorkerTask，保存 worker_task_handle -> 创建 MonitorTask，保存 monitor_task_handle
```

## WorkerTask

完整函数：

```c
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
```

横向流程：

```text
WorkerTask 运行 -> 使用 local_buffer[256] 消耗一部分任务栈 -> 翻转 LED3 -> 阻塞 500ms -> 重复
```

这里的 `local_buffer[256]` 是故意设计的：

```text
它是局部变量，放在 WorkerTask 的任务栈里。数组越大，WorkerTask 的栈剩余水位越低。
```

所以它会影响：

```text
worker_stack_free_words
```

## MonitorTask

完整函数：

```c
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
```

横向流程：

```text
MonitorTask 运行 -> 延时 2000ms -> 打印 memory check -> 申请一块 heap -> 打印 heap/stack 信息 -> 重复
```

## ConsumeHeapStep()

本课为了让 heap 消耗过程更直观，故意让 `MonitorTask` 每 2000ms 申请一块 FreeRTOS heap，并且不释放。

关键定义：

```c
#define HEAP_CONSUME_STEP_BYTES 2600
#define HEAP_CONSUME_MAX_STEPS 5

static void *heap_blocks[HEAP_CONSUME_MAX_STEPS];
static uint8_t heap_block_count = 0;
```

含义：

```text
每次申请 2600 bytes。
最多申请 5 次。
用 heap_blocks[] 保存每次申请到的指针，表示这些内存仍然被占用。
```

完整函数：

```c
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
```

横向流程：

```text
检查是否已经申请 5 次 -> 没到上限就 pvPortMalloc(2600) -> 申请成功后保存指针 -> heap_block_count +1
```

这样做的目的不是推荐在项目里制造内存泄漏，而是为了观察：

```text
FreeRTOS heap 会随着动态分配逐步下降。
```

## SendMemoryLog()

关键代码：

```c
size_t heap_free = xPortGetFreeHeapSize();
size_t heap_min = xPortGetMinimumEverFreeHeapSize();
UBaseType_t worker_stack_free = uxTaskGetStackHighWaterMark(worker_task_handle);
UBaseType_t monitor_stack_free = uxTaskGetStackHighWaterMark(monitor_task_handle);
```

逐项理解：

```text
xPortGetFreeHeapSize()：当前还剩多少 FreeRTOS heap。
xPortGetMinimumEverFreeHeapSize()：系统运行以来，heap 最少剩过多少。
uxTaskGetStackHighWaterMark(worker_task_handle)：WorkerTask 历史上最少还剩多少栈空间。
uxTaskGetStackHighWaterMark(monitor_task_handle)：MonitorTask 历史上最少还剩多少栈空间。
```

注意：

```text
uxTaskGetStackHighWaterMark() 返回单位是 word，不是 byte。
```

在 Cortex-M3 上：

```text
1 word = 4 bytes
```

所以如果看到：

```text
worker_stack_free_words=80
```

大约表示：

```text
WorkerTask 历史最紧张时还剩 80 * 4 = 320 bytes 栈空间
```

## malloc failed hook

代码：

```c
void vApplicationMallocFailedHook(void)
{
  Usart_SendString(DEBUG_USARTx, "malloc failed hook\r\n");
  while (1)
  {
  }
}
```

如果 FreeRTOS heap 不够，比如 `xTaskCreate()` 分配任务失败，FreeRTOS 可以调用这个 hook。

它的作用是：

```text
让内存分配失败变成可观察现象，而不是静默失败。
```

## stack overflow hook

代码：

```c
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
```

如果 FreeRTOS 检测到任务栈溢出，会调用这个 hook。

它的作用是：

```text
告诉你哪个任务的栈可能出问题。
```

注意：

```text
栈溢出检测不是万能的。
真实项目里仍然要主动查看 high water mark，给任务留足栈余量。
```

## 预期串口现象

Reset 后先输出：

```text
lesson-23-memory-stack start
```

随后可以看到类似：

```text
[tick 2000] MonitorTask: memory check
  heap_blocks=1, step_bytes=2600
  heap_free=xxxxx, heap_min=xxxxx
  worker_stack_free_words=xx, monitor_stack_free_words=xx
[tick 4000] MonitorTask: memory check
  heap_blocks=2, step_bytes=2600
  heap_free=xxxxx, heap_min=xxxxx
  worker_stack_free_words=xx, monitor_stack_free_words=xx
```

LED3 会每 500ms 翻转一次。

## 如何分析这些数字

`heap_free`：

```text
当前剩余 heap。
```

如果后续创建更多任务、队列、信号量，通常会下降。本课里 `MonitorTask` 每 2000ms 调用一次 `pvPortMalloc(2600)`，所以它会逐步下降。

`heap_min`：

```text
系统运行以来历史最低 heap 剩余量。
```

它比 `heap_free` 更适合判断系统是否曾经接近内存耗尽。

本课更直观的观察点是：

```text
tick 2000：heap_blocks=1，heap_free 下降一次。
tick 4000：heap_blocks=2，heap_free 再下降一次。
tick 6000：heap_blocks=3，heap_free 再下降一次。
tick 8000：heap_blocks=4，heap_free 再下降一次。
tick 10000：heap_blocks=5，heap_free 接近最低。
tick 12000 之后：heap_blocks 保持 5，heap_free 基本稳定。
```

也就是说：

```text
前 10000 tick 观察 heap 被逐步占用。
10000 tick 后观察系统在低 heap 剩余状态下继续运行。
```

`worker_stack_free_words` 和 `monitor_stack_free_words`：

```text
任务栈历史最低剩余 word 数。
```

数值太小，说明任务栈余量不足。

如果长期接近 0：

```text
应该增大 xTaskCreate() 的栈大小参数。
```

## 实际现象分析

你观察到的日志：

```text
[tick 2000] MonitorTask: memory check
  heap_blocks=1, step_bytes=2600
  heap_free=11360, heap_min=11360
  worker_stack_free_words=89, monitor_stack_free_words=184
[tick 4015] MonitorTask: memory check
  heap_blocks=2, step_bytes=2600
  heap_free=8752, heap_min=8752
  worker_stack_free_words=89, monitor_stack_free_words=184
[tick 6029] MonitorTask: memory check
  heap_blocks=3, step_bytes=2600
  heap_free=6144, heap_min=6144
  worker_stack_free_words=89, monitor_stack_free_words=182
[tick 8043] MonitorTask: memory check
  heap_blocks=4, step_bytes=2600
  heap_free=3536, heap_min=3536
  worker_stack_free_words=89, monitor_stack_free_words=182
[tick 10057] MonitorTask: memory check
  heap_blocks=5, step_bytes=2600
  heap_free=928, heap_min=928
  worker_stack_free_words=89, monitor_stack_free_words=182
```

这正是本课想观察的效果。

heap 每次下降：

```text
11360 - 8752 = 2608
8752 - 6144 = 2608
6144 - 3536 = 2608
3536 - 928  = 2608
```

代码里每次申请：

```c
#define HEAP_CONSUME_STEP_BYTES 2600
```

实际每次少了 2608 bytes，是因为 `heap_4.c` 分配内存时除了用户请求的 2600 bytes，还会有：

```text
内存块管理信息
内存对齐开销
```

所以 heap 下降值略大于 2600 是正常的。

`heap_min` 跟着 `heap_free` 一起下降：

```text
11360 -> 8752 -> 6144 -> 3536 -> 928
```

说明系统运行以来的历史最低 heap 剩余不断被刷新。

到 `heap_blocks=5` 后：

```text
heap_free=928
heap_min=928
```

后续不再下降，因为代码限制了最多申请 5 次：

```c
#define HEAP_CONSUME_MAX_STEPS 5
```

所以完整理解是：

```text
前 10000 tick：每 2000 tick 申请一次 heap，heap_free 持续下降。
10000 tick 后：达到 5 次申请上限，不再申请，heap_free 稳定在 928。
```

`worker_stack_free_words=89` 表示 WorkerTask 历史最低还剩：

```text
89 words = 89 * 4 = 356 bytes
```

WorkerTask 创建时栈大小是：

```text
180 words = 720 bytes
```

所以 WorkerTask 历史最大大约用掉：

```text
180 - 89 = 91 words = 364 bytes
```

这说明 `local_buffer[256]` 确实压低了 WorkerTask 的任务栈水位。

本课最终要分清：

```text
heap_free 下降：来自 pvPortMalloc() 动态申请 heap。
worker_stack_free_words 下降：来自 WorkerTask 局部变量和函数调用消耗任务栈。
```

## 本课结论

这一课要建立五个判断：

```text
1. FreeRTOS heap 用来动态分配任务、队列、信号量等内核对象。
2. 每个任务都有自己的任务栈，局部变量和函数调用会消耗任务栈。
3. xPortGetFreeHeapSize() 可以查看当前 heap 剩余。
4. uxTaskGetStackHighWaterMark() 可以查看任务栈历史最低剩余。
5. malloc failed hook 和 stack overflow hook 是 RTOS 项目里重要的故障观测点。
```

一句话：

```text
写 FreeRTOS 项目不能只看功能能不能跑，还要看 heap 和 stack 有没有足够余量。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd '/home/yang/stm32-FreeRTOS/lessons/lesson-23-memory-stack/27-FreeRTOS内存与任务栈'
make clean
make
```

生成产物：

```text
build/lesson-23-memory-stack.elf
build/lesson-23-memory-stack.hex
build/lesson-23-memory-stack.bin
build/lesson-23-memory-stack.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
