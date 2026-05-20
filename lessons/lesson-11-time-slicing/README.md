# Lesson 11：同优先级任务与时间片轮转

## 本课目标

上一课验证的是不同优先级任务的调度顺序：

```text
高优先级任务 -> 中优先级任务 -> 低优先级任务
```

这一课验证另一个问题：

```text
如果两个任务优先级相同，而且都一直处于就绪态，FreeRTOS 会怎么调度它们？
```

本课使用两个同优先级任务：

```text
TaskA：优先级 2，不主动 vTaskDelay()
TaskB：优先级 2，不主动 vTaskDelay()
```

在 `configUSE_TIME_SLICING = 1` 的情况下，FreeRTOS 会在系统 tick 到来时，让同优先级的就绪任务轮流获得 CPU。

## 对比上一个 lesson

对比 Lesson 10，本课主要变化仍然在 `User/main.c`。

上一课：

```text
HighPriorityTask：优先级 3，运行后 vTaskDelay(1000ms)
MediumLedTask：优先级 2，运行后 vTaskDelay(1000ms)
LowPriorityTask：优先级 1，运行后 vTaskDelay(1000ms)
```

本课：

```text
TaskA：优先级 2，不主动阻塞
TaskB：优先级 2，不主动阻塞
```

这一课故意不让 `TaskA` 和 `TaskB` 调用 `vTaskDelay()`。原因是：如果任务主动 delay，观察到的是阻塞调度；如果任务一直就绪，才更适合观察同优先级时间片轮转。

## FreeRTOSConfig.h 关键配置

本工程中已经开启：

```c
#define configUSE_PREEMPTION    1
#define configUSE_TIME_SLICING  1
```

含义：

```text
configUSE_PREEMPTION = 1：启用抢占式调度。
configUSE_TIME_SLICING = 1：同优先级任务之间启用时间片轮转。
```

如果关闭时间片轮转，一个不主动阻塞的同优先级任务可能长时间占住 CPU；另一个同优先级任务就不容易运行。

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印 lesson-11-time-slicing start -> xTaskCreate(TaskA, 优先级2) -> xTaskCreate(TaskB, 优先级2) -> vTaskStartScheduler()
```

`TaskA` 的横向流程：

```text
TaskA 运行 -> 判断是否到达下一次打印 tick -> 到达则翻转蓝灯并打印 -> 继续保持就绪态
```

`TaskB` 的横向流程：

```text
TaskB 运行 -> 判断是否到达下一次打印 tick -> 到达则打印 -> 继续保持就绪态
```

两个任务都没有 `vTaskDelay()`，所以它们都长期处于就绪态。它们能交替运行，靠的是 tick 中断触发调度器进行同优先级时间片轮转。

## 预期串口现象

Reset 后，串口会先输出：

```text
lesson-11-time-slicing start
```

随后可以看到类似日志：

```text
[tick 0] TaskA: same priority, still ready
[tick 1] TaskB: same priority, still ready
[tick 1000] TaskA: same priority, still ready
[tick 1001] TaskB: same priority, still ready
[tick 2000] TaskA: same priority, still ready
[tick 2001] TaskB: same priority, still ready
```

实际 tick 可能会有几毫秒偏差，因为串口发送本身会消耗时间。重点观察：

```text
TaskA 和 TaskB 都能持续输出。
两个任务优先级相同。
两个任务都没有主动 vTaskDelay()。
这说明同优先级任务正在通过时间片轮转获得 CPU。
```

## 为什么没有疯狂刷屏

虽然 `TaskA` 和 `TaskB` 一直在运行，但代码里使用 `next_log_tick` 控制打印频率：

```text
任务一直就绪 -> 每次运行都检查当前 tick -> 只有到达下一次打印时间才输出串口日志
```

所以程序既能保持任务长期就绪，又不会把串口助手刷爆。

## 课堂讨论记录

本课烧录后，实际观察到的串口日志可能是：

```text
lesson-11-time-slicing start
[tick 0] TaskB: same priority, still ready
[tick 3] TaskA: same priority, still ready
[tick 1007] TaskB: same priority, still ready
[tick 1011] TaskA: same priority, still ready
[tick 2015] TaskB: same priority, still ready
[tick 2019] TaskA: same priority, still ready
```

这个现象是正确的。它说明：

```text
TaskB 在持续运行。
TaskA 也在持续运行。
TaskA 和 TaskB 优先级相同。
TaskA 和 TaskB 都没有 vTaskDelay()。
```

所以这份日志和本课主题的联系是：

```text
两个同优先级任务都长期处于就绪态 -> 两个任务都没有主动阻塞 -> 两个任务却都能持续输出 -> FreeRTOS 正在通过时间片轮转让它们共享 CPU
```

### 为什么 TaskB 总是在 TaskA 前面

多次复位后如果总是看到：

```text
TaskB -> TaskA -> TaskB -> TaskA
```

这不代表 `TaskB` 的优先级更高。两个任务的优先级都是 2。

更准确的理解是：

```text
FreeRTOS 同优先级任务不是随机调度，而是在同一个 ready list 中轮转。
调度器启动时，链表游标、任务插入位置和首次调度时机共同决定谁先运行。
TaskB 第一次先打印后，它的 next_log_tick 也先建立。
后续每一轮 TaskB 的打印时间点就稳定早于 TaskA。
```

因此，`TaskB` 总在前面只是当前代码和当前启动时序下的稳定结果，不影响本课结论。

### TaskA 和 TaskB 实际在做什么

`TaskA` 的核心逻辑：

```text
while(1) -> 读取当前 tick -> 判断是否到达 next_log_tick -> 到达则翻转蓝灯并打印 -> 更新 next_log_tick -> 继续循环
```

`TaskB` 的核心逻辑：

```text
while(1) -> 读取当前 tick -> 判断是否到达 next_log_tick -> 到达则打印 -> 更新 next_log_tick -> 继续循环
```

它们的重点不是“每秒执行一次”。实际上，两个任务一直在运行，只是只有到达 `next_log_tick` 时才输出日志。

更准确地说：

```text
串口日志不是完整调度轨迹。
串口日志只是任务持续获得 CPU 的抽样证据。
```

### 和时间片轮转的关系

本课的关键证据链是：

```text
TaskA/TaskB 同优先级
-> 都没有 vTaskDelay()
-> 都长期保持就绪态
-> 如果没有同优先级时间片轮转，其中一个任务可能长期占住 CPU
-> 现在两个任务都能持续输出
-> 说明 tick 正在推动同优先级任务之间的时间片切换
```

对比上一课：

```text
Lesson 10：任务运行 -> vTaskDelay() -> 主动阻塞 -> 其他任务运行
Lesson 11：任务运行 -> 不主动阻塞 -> 仍然就绪 -> 靠时间片切换到另一个同优先级任务
```

## 本课结论

这一课要建立三个判断：

```text
1. 不同优先级任务：优先级高者先运行。
2. 相同优先级任务：在时间片轮转开启时，可以轮流运行。
3. vTaskDelay() 不是任务切换的唯一来源；tick 中断也可以触发调度器在同优先级任务间切换。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd '/home/yang/stm32-FreeRTOS/lessons/lesson-11-time-slicing/15-FreeRTOS时间片轮转'
make clean
make
```

生成产物：

```text
build/lesson-11-time-slicing.elf
build/lesson-11-time-slicing.hex
build/lesson-11-time-slicing.bin
build/lesson-11-time-slicing.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
