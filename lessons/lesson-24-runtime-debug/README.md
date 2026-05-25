# Lesson 24：FreeRTOS 运行状态与调试

## 本课目标

上一课 Lesson 23 学的是 heap 和任务栈水位。

这一课继续向“工程调试”靠近：

```text
系统里现在有多少任务？
每个任务优先级是多少？
每个任务当前处于什么状态？
每个任务栈还剩多少？
heap 还剩多少？
```

本课目标是观察：

```text
FastTask 每 500ms 翻转 LED3。
SlowTask 每 3000ms 打印一次 periodic work。
MonitorTask 每 2000ms 打印系统运行状态报告。
```

## 对比上一个 lesson

对比 Lesson 23，本课不再故意持续申请 heap。

本课变化：

```text
User/main.c 新增 FastTask()。
User/main.c 新增 SlowTask()。
User/main.c 新增 MonitorTask()。
User/main.c 新增 SendRuntimeReport()。
User/main.c 新增 SendTaskStatus()。
User/main.c 新增 TaskStateToString()。
User/FreeRTOSConfig.h 打开 INCLUDE_uxTaskPriorityGet。
User/FreeRTOSConfig.h 打开 INCLUDE_eTaskGetState。
Makefile 目标名改为 lesson-24-runtime-debug。
```

## 为什么需要运行状态输出

FreeRTOS 项目里，问题经常不是“编译不过”，而是：

```text
某个任务没有运行。
某个任务优先级设置不合理。
某个任务一直阻塞。
某个任务栈快不够了。
heap 被消耗太多。
```

所以调试 RTOS 项目时，需要周期性观察系统状态。

本课做的就是一个简化版运行状态报告：

```text
任务数量
heap 剩余
每个任务的优先级
每个任务的状态
每个任务的栈水位
```

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印 lesson-24-runtime-debug start -> CreateTasks() -> vTaskStartScheduler()
```

`CreateTasks()` 的横向流程：

```text
创建 FastTask，优先级 2 -> 创建 SlowTask，优先级 1 -> 创建 MonitorTask，优先级 3
```

## 三个任务

`FastTask`：

```text
每 500ms 翻转 LED3。
优先级 2。
```

`SlowTask`：

```text
每 3000ms 打印 periodic work。
优先级 1。
```

`MonitorTask`：

```text
每 2000ms 打印运行状态报告。
优先级 3。
```

优先级设计：

```text
MonitorTask 优先级最高，方便准时输出调试信息。
FastTask 中等优先级，模拟周期任务。
SlowTask 最低优先级，模拟普通后台任务。
```

## SendRuntimeReport()

关键代码：

```c
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
```

横向流程：

```text
读取当前 tick -> 暂停调度器避免多任务交叉打印 -> 打印任务数量和 heap -> 分别打印 fast/slow/monitor 状态 -> 恢复调度器
```

## SendTaskStatus()

关键代码：

```c
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
```

横向流程：

```text
传入任务句柄 -> 查询优先级 -> 查询任务状态 -> 查询栈水位 -> 串口输出
```

## 几个调试 API

`uxTaskGetNumberOfTasks()`：

```text
返回当前 FreeRTOS 管理的任务数量。
```

注意这里通常会包含：

```text
用户创建的任务
Idle Task
```

如果启用了软件定时器，还可能包含：

```text
Timer Service Task
```

`uxTaskPriorityGet(task_handle)`：

```text
返回指定任务当前优先级。
```

本课预期：

```text
fast priority=2
slow priority=1
monitor priority=3
```

`eTaskGetState(task_handle)`：

```text
返回指定任务当前状态。
```

常见状态：

```text
Running：正在运行。
Ready：就绪，等待 CPU。
Blocked：阻塞，通常在 vTaskDelay() 或等待事件。
Suspended：挂起。
Deleted：已删除但资源可能还没清理完。
```

`uxTaskGetStackHighWaterMark(task_handle)`：

```text
返回指定任务历史最低剩余栈空间。
```

单位：

```text
word，不是 byte。
```

Cortex-M3 上：

```text
1 word = 4 bytes
```

## TaskStateToString()

FreeRTOS 的 `eTaskGetState()` 返回的是枚举值，不适合直接串口打印。

所以本课写了：

```c
static const char *TaskStateToString(eTaskState state)
```

把枚举值转换成字符串：

```text
eRunning   -> Running
eReady     -> Ready
eBlocked   -> Blocked
eSuspended -> Suspended
eDeleted   -> Deleted
```

## 预期串口现象

Reset 后先输出：

```text
lesson-24-runtime-debug start
```

随后可以看到类似：

```text
[tick 0] SlowTask: periodic work
[tick 2000] Runtime report
  task_count=4, heap_free=xxxxx, heap_min=xxxxx
  fast: priority=2, state=Blocked, stack_free_words=xxx
  slow: priority=1, state=Blocked, stack_free_words=xxx
  monitor: priority=3, state=Running, stack_free_words=xxx
[tick 3000] SlowTask: periodic work
[tick 4000] Runtime report
```

为什么 `task_count` 可能是 4？

```text
用户创建了 fast、slow、monitor 三个任务。
FreeRTOS 还会自动创建 Idle Task。
```

为什么大多数任务是 `Blocked`？

```text
FastTask 在 vTaskDelay(500) 中等待。
SlowTask 在 vTaskDelay(3000) 中等待。
MonitorTask 打印报告时自己正在运行，所以是 Running。
```

## 课堂总结

这一课没有引入新的调度机制，也没有新的任务通信模型。

它的本质是学习几个 FreeRTOS 系统观测接口：

```text
uxTaskGetNumberOfTasks()：当前系统有多少任务。
uxTaskPriorityGet()：某个任务当前优先级是多少。
eTaskGetState()：某个任务当前处于什么状态。
uxTaskGetStackHighWaterMark()：某个任务历史最低还剩多少栈。
xPortGetFreeHeapSize()：当前 heap 还剩多少。
xPortGetMinimumEverFreeHeapSize()：系统运行以来 heap 最少剩过多少。
```

这些接口主要用于回答真实项目里的调试问题：

```text
任务为什么不运行？
任务是不是一直阻塞？
任务优先级是否设置合理？
任务栈是否还有余量？
heap 是否接近耗尽？
```

所以本课一句话总结是：

```text
Lesson 24 不是学新功能，而是学会向 FreeRTOS 查询系统状态。
```

## 本课结论

这一课要建立五个判断：

```text
1. RTOS 项目需要运行状态观测，不只是功能能跑。
2. uxTaskGetNumberOfTasks() 可以查看当前任务数量。
3. uxTaskPriorityGet() 可以查看任务优先级。
4. eTaskGetState() 可以查看任务状态。
5. uxTaskGetStackHighWaterMark() 可以查看任务栈余量。
```

一句话：

```text
调试 FreeRTOS 项目时，要能回答“有哪些任务、状态如何、栈够不够、heap 还剩多少”。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd '/home/yang/stm32-FreeRTOS/lessons/lesson-24-runtime-debug/28-FreeRTOS运行状态与调试'
make clean
make
```

生成产物：

```text
build/lesson-24-runtime-debug.elf
build/lesson-24-runtime-debug.hex
build/lesson-24-runtime-debug.bin
build/lesson-24-runtime-debug.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
