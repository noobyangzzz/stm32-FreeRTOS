# Lesson 20：软件定时器 Software Timer

## 本课目标

上一课 Lesson 19 学的是事件组：

```text
多个任务设置事件 bit -> 等待任务等多个条件满足 -> 等待任务继续执行
```

这一课学习软件定时器：

```text
创建一个定时器 -> 启动定时器 -> 时间到后 FreeRTOS 调用回调函数
```

本课目标是观察：

```text
软件定时器每 1000ms 到期一次。
TimerCallback() 被 FreeRTOS 自动调用。
TimerCallback() 打印 tick 并翻转 LED3。
```

## 对比上一个 lesson

对比 Lesson 19，本课不再使用事件组，也不创建 EventTaskA/EventTaskB/WaitTask。

本课变化：

```text
User/main.c 引入 timers.h。
User/main.c 新增 TimerHandle_t periodic_timer。
User/main.c 新增 TimerCallback()。
User/main.c 使用 xTimerCreate() 创建软件定时器。
User/main.c 使用 xTimerStart() 启动软件定时器。
User/FreeRTOSConfig.h 打开 configUSE_TIMERS。
Makefile 新增 FreeRTOS/Source/timers.c。
Makefile 目标名改为 lesson-20-software-timer。
```

这部分实现的功能：

```text
periodic_timer：每 1000ms 触发一次。
TimerCallback()：定时器到期后执行，翻转 LED3 并打印串口日志。
```

## 软件定时器是什么

软件定时器不是 STM32 的 TIM 外设定时器。

它是 FreeRTOS 内核提供的一个软件机制：

```text
FreeRTOS tick 推进系统时间 -> 定时器到期 -> Timer Service Task 执行回调函数
```

所以要注意：

```text
软件定时器回调函数不是中断函数。
软件定时器回调函数运行在 FreeRTOS 的 Timer Service Task 中。
```

这意味着：

```text
TimerCallback() 里不能长时间阻塞。
TimerCallback() 里不要写死循环。
TimerCallback() 里不要做很重的工作。
```

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印 lesson-20-software-timer start -> xTimerCreate() -> CheckTimerCreated() -> xTimerStart() -> CheckTimerStarted() -> vTaskStartScheduler()
```

完整关键代码：

```c
periodic_timer = xTimerCreate("periodic",
                              pdMS_TO_TICKS(SOFTWARE_TIMER_PERIOD_MS),
                              pdTRUE,
                              NULL,
                              TimerCallback);
CheckTimerCreated(periodic_timer);

CheckTimerStarted(xTimerStart(periodic_timer, 0));
```

## xTimerCreate()

参数指南：

```c
periodic_timer = xTimerCreate(
    "periodic",                              /* 定时器名字，主要用于调试 */
    pdMS_TO_TICKS(SOFTWARE_TIMER_PERIOD_MS), /* 定时周期，这里是 1000ms */
    pdTRUE,                                  /* 是否自动重载；pdTRUE 表示周期定时器 */
    NULL,                                    /* 定时器 ID，本课暂时不用 */
    TimerCallback                            /* 定时器到期后调用的回调函数 */
);
```

横向理解：

```text
申请一个软件定时器对象 -> 设置周期 1000ms -> 设置为周期模式 -> 绑定 TimerCallback()
```

其中 `pdTRUE` 很关键：

```text
pdTRUE：周期定时器，到期后会自动重新开始计时。
pdFALSE：单次定时器，只触发一次。
```

## xTimerStart()

代码：

```c
CheckTimerStarted(xTimerStart(periodic_timer, 0));
```

横向理解：

```text
向 Timer Service Task 发送“启动这个定时器”的命令 -> 命令发送成功 -> 定时器开始计时
```

第二个参数 `0` 表示：

```text
如果发送启动命令时队列满了，不阻塞等待。
```

本课是在调度器启动前调用 `xTimerStart()`，FreeRTOS 会先把启动命令放进定时器命令队列，等调度器启动后再由 Timer Service Task 处理。

## TimerCallback()

完整函数：

```c
static void TimerCallback(TimerHandle_t timer)
{
  (void)timer;

  LED3_TOGGLE;
  SendTimerLog("TimerCallback", "periodic timer expired");
}
```

横向流程：

```text
软件定时器到期 -> FreeRTOS 调用 TimerCallback() -> 翻转 LED3 -> 打印 tick 日志 -> 回调函数返回
```

这里的 `timer` 参数是当前到期的定时器句柄。本课只有一个定时器，所以暂时不用它。

## Timer Service Task

启用软件定时器后，FreeRTOS 内部会创建一个定时器服务任务。

相关配置：

```c
#define configUSE_TIMERS             1
#define configTIMER_TASK_PRIORITY    2
#define configTIMER_QUEUE_LENGTH     5
#define configTIMER_TASK_STACK_DEPTH configMINIMAL_STACK_SIZE
```

横向理解：

```text
configUSE_TIMERS = 1 -> FreeRTOS 编译软件定时器功能 -> 调度器启动时创建 Timer Service Task -> Timer Service Task 负责执行 TimerCallback()
```

所以软件定时器的完整链路是：

```text
xTimerCreate() 创建定时器 -> xTimerStart() 发送启动命令 -> vTaskStartScheduler() 启动调度器 -> Timer Service Task 处理启动命令 -> tick 到期 -> Timer Service Task 调用 TimerCallback()
```

## 预期串口现象

Reset 后先输出：

```text
lesson-20-software-timer start
```

随后大约每 1000 tick 输出一次：

```text
[tick 1000] TimerCallback: periodic timer expired
[tick 2000] TimerCallback: periodic timer expired
[tick 3000] TimerCallback: periodic timer expired
[tick 4000] TimerCallback: periodic timer expired
```

同时 LED3 蓝灯会每 1000ms 翻转一次。

## 课堂讨论总结

### 和 vTaskDelay() 的区别

`vTaskDelay()` 或者常说的 `os_delay`，本质是：

```text
某个任务自己主动阻塞一段时间，时间到了以后这个任务再继续运行。
```

典型结构：

```c
while (1)
{
  LED3_TOGGLE;
  vTaskDelay(pdMS_TO_TICKS(1000));
}
```

横向链路：

```text
任务运行 -> 翻转 LED3 -> vTaskDelay(1000ms) -> 当前任务进入阻塞态 -> 1000ms 到期 -> 当前任务重新就绪 -> 当前任务继续运行
```

软件定时器的本质是：

```text
注册一个到期回调函数，由 FreeRTOS 内部的 Timer Service Task 到点调用。
```

本课链路：

```text
xTimerCreate() 创建定时器 -> xTimerStart() 启动定时器 -> FreeRTOS tick 计时 -> 定时器到期 -> Timer Service Task 调用 TimerCallback()
```

所以二者的核心区别是：

```text
vTaskDelay()：任务自己睡眠，醒来后继续执行。
软件定时器：你注册回调函数，FreeRTOS 到点帮你调用。
```

### 如何理解本课现象

实际串口现象：

```text
lesson-20-software-timer start
[tick 1000] TimerCallback: periodic timer expired
[tick 2000] TimerCallback: periodic timer expired
[tick 3000] TimerCallback: periodic timer expired
```

横向理解：

```text
Reset -> main() 创建 periodic_timer -> xTimerStart() 启动定时器 -> vTaskStartScheduler() 启动调度器 -> Timer Service Task 开始管理定时器 -> 每 1000 tick 到期一次 -> 调用 TimerCallback() -> 打印日志并翻转 LED3
```

因为本课配置：

```text
configTICK_RATE_HZ = 1000
```

所以：

```text
1000 tick = 1000ms = 1s
```

日志每次出现在 `1000、2000、3000...`，说明：

```text
周期定时器创建成功。
定时器启动成功。
configUSE_TIMERS 已打开。
timers.c 已参与编译。
Timer Service Task 正常工作。
TimerCallback() 正常被周期调用。
```

一句话：

```text
软件定时器像是设置一个“到点执行的定时任务”，但真正被调度的是 FreeRTOS 内部的 Timer Service Task，TimerCallback() 只是它到点执行的回调函数。
```

## 本课结论

这一课要建立四个判断：

```text
1. 软件定时器是 FreeRTOS 内核提供的定时机制，不是 STM32 TIM 外设。
2. xTimerCreate() 负责创建定时器对象。
3. xTimerStart() 负责启动定时器。
4. TimerCallback() 运行在 Timer Service Task 中，不是中断服务函数。
```

一句话：

```text
软件定时器适合做周期性、轻量级、非阻塞的定时触发动作。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd '/home/yang/stm32-FreeRTOS/lessons/lesson-20-software-timer/24-FreeRTOS软件定时器'
make clean
make
```

生成产物：

```text
build/lesson-20-software-timer.elf
build/lesson-20-software-timer.hex
build/lesson-20-software-timer.bin
build/lesson-20-software-timer.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
