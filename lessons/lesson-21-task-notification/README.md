# Lesson 21：任务通知 Task Notification

## 本课目标

上一课 Lesson 20 学的是软件定时器：

```text
创建软件定时器 -> 启动定时器 -> Timer Service Task 到期调用 TimerCallback()
```

这一课学习任务通知：

```text
一个任务直接通知另一个任务。
```

本课目标是观察：

```text
NotifyTask 每 1000ms 发送一次任务通知。
WaitTask 阻塞等待任务通知。
WaitTask 收到通知后打印日志并翻转 LED3。
```

## 对比上一个 lesson

对比 Lesson 20，本课不再使用软件定时器。

本课变化：

```text
User/main.c 移除 timers.h。
User/main.c 移除 TimerHandle_t 和 TimerCallback()。
User/main.c 新增 wait_task_handle。
User/main.c 新增 NotifyTask() 和 WaitTask()。
User/main.c 使用 xTaskNotifyGive() 发送通知。
User/main.c 使用 ulTaskNotifyTake() 等待通知。
User/FreeRTOSConfig.h 关闭 configUSE_TIMERS。
Makefile 移除 FreeRTOS/Source/timers.c。
Makefile 目标名改为 lesson-21-task-notification。
```

这部分实现的功能：

```text
NotifyTask：每 1000ms 通知 WaitTask。
WaitTask：没有通知时阻塞；收到通知后继续运行。
```

## 任务通知是什么

任务通知可以理解成：

```text
FreeRTOS 给每个任务 TCB 内置了一个通知值。
```

所以它不需要额外创建队列、信号量或事件组对象。

对比二值信号量：

```text
二值信号量：创建 SemaphoreHandle_t -> 任务 give -> 任务 take。
任务通知：保存目标任务 TaskHandle_t -> 直接通知目标任务 -> 目标任务等待通知。
```

一句话：

```text
任务通知是直接发给某个任务的轻量级同步机制。
```

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印 lesson-21-task-notification start -> CreateTasks() -> vTaskStartScheduler()
```

`CreateTasks()` 的横向流程：

```text
创建 WaitTask 并保存 wait_task_handle -> 创建 NotifyTask
```

这里先创建 `WaitTask`，是因为 `NotifyTask` 要通过 `wait_task_handle` 找到被通知的目标任务。

## TaskHandle_t

本课代码：

```c
static TaskHandle_t wait_task_handle = NULL;
```

横向理解：

```text
WaitTask 创建成功 -> FreeRTOS 返回这个任务的句柄 -> 保存到 wait_task_handle -> NotifyTask 通过这个句柄通知 WaitTask
```

任务通知不是广播。

它必须知道：

```text
我要通知哪一个任务。
```

所以任务句柄是这节课的关键。

## NotifyTask

完整函数：

```c
static void NotifyTask(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
    SendTaskLog("NotifyTask", "give notification");
    xTaskNotifyGive(wait_task_handle);
  }
}
```

横向流程：

```text
NotifyTask 运行 -> 延时 1000ms -> 打印 give notification -> 通知 WaitTask -> 重复
```

关键函数：

```c
xTaskNotifyGive(wait_task_handle);
```

含义：

```text
给 wait_task_handle 对应的任务发送一次通知。
```

在本课里，它的作用类似：

```text
给 WaitTask 一个“可以继续运行了”的信号。
```

## WaitTask

完整函数：

```c
static void WaitTask(void *argument)
{
  uint32_t notification_value;

  (void)argument;

  while (1)
  {
    notification_value = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    (void)notification_value;

    LED3_TOGGLE;
    SendTaskLog("WaitTask", "take notification");
  }
}
```

横向流程：

```text
WaitTask 运行 -> ulTaskNotifyTake() 阻塞等待通知 -> 收到通知后返回 -> 翻转 LED3 -> 打印 take notification -> 继续下一轮等待
```

## ulTaskNotifyTake()

参数指南：

```c
notification_value = ulTaskNotifyTake(
    pdTRUE,        /* 收到通知后是否清零通知值；pdTRUE 表示清零 */
    portMAX_DELAY  /* 最长等待时间；portMAX_DELAY 表示一直阻塞等待 */
);
```

第一个参数：

```text
pdTRUE：收到通知后，把通知值清零。适合把任务通知当二值信号量用。
pdFALSE：收到通知后，把通知值减 1。适合把任务通知当计数信号量用。
```

第二个参数：

```text
portMAX_DELAY：没有通知就一直阻塞，不占用 CPU 空转。
```

## 和二值信号量的关系

Lesson 16 的二值信号量流程：

```text
xSemaphoreCreateBinary() -> SignalTask give -> WaitTask take
```

Lesson 21 的任务通知流程：

```text
保存 WaitTask 句柄 -> NotifyTask xTaskNotifyGive() -> WaitTask ulTaskNotifyTake()
```

相似点：

```text
都可以实现“一个任务发信号，另一个任务等待信号”。
```

不同点：

```text
二值信号量需要单独创建信号量对象。
任务通知直接使用目标任务 TCB 里的通知值。
```

所以在“一对一通知任务”的场景里，任务通知通常更轻量。

## 预期串口现象

Reset 后先输出：

```text
lesson-21-task-notification start
```

随后可以看到类似：

```text
[tick 1000] NotifyTask: give notification
[tick 1003] WaitTask: take notification
[tick 2003] NotifyTask: give notification
[tick 2006] WaitTask: take notification
[tick 3006] NotifyTask: give notification
[tick 3009] WaitTask: take notification
```

重点观察：

```text
NotifyTask 每 1000ms 发送一次通知。
WaitTask 只有收到通知后才运行。
WaitTask 没有通知时处于阻塞态，不占用 CPU。
```

## 课堂讨论总结

### ulTaskNotifyTake() 的含义

`ulTaskNotifyTake()` 可以理解成：

```text
当前任务等待别人给自己发送任务通知。
```

函数名拆开看：

```text
ul      -> unsigned long，表示返回值类型
Task    -> 任务
Notify  -> 通知
Take    -> 取走通知
```

本课代码：

```c
notification_value = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
```

横向理解：

```text
WaitTask 等待自己的通知值变成非 0 -> 没有通知就阻塞 -> 收到通知后返回 -> 因为 pdTRUE，把通知值清零
```

参数含义：

```text
pdTRUE：收到通知后清零通知值，适合把任务通知当二值信号量用。
pdFALSE：收到通知后通知值减 1，适合把任务通知当计数信号量用。
portMAX_DELAY：没有通知就一直阻塞等待，不占用 CPU 空转。
```

### 三次通知的完整链路

系统启动：

```text
main() -> 创建 WaitTask 并保存 wait_task_handle -> 创建 NotifyTask -> vTaskStartScheduler()
```

在当前代码里：

```text
WaitTask 优先级 = 3
NotifyTask 优先级 = 2
```

所以两个任务都就绪时，WaitTask 会先运行：

```text
WaitTask 运行 -> ulTaskNotifyTake() -> 当前没有通知 -> WaitTask 阻塞 -> NotifyTask 开始运行
```

第 1 次：

```text
NotifyTask 延时 1000ms -> 打印 give notification -> xTaskNotifyGive(wait_task_handle) -> WaitTask 的通知值 +1 -> WaitTask 从阻塞态变为就绪态 -> WaitTask 被调度运行 -> ulTaskNotifyTake() 返回并清零通知值 -> LED3_TOGGLE -> 打印 take notification -> WaitTask 再次阻塞等待
```

第 2 次：

```text
NotifyTask 下一轮延时到期 -> 再次 xTaskNotifyGive() -> WaitTask 再次被唤醒 -> WaitTask 取走通知并清零 -> 执行动作 -> 再次阻塞
```

第 3 次：

```text
NotifyTask 第三次发送通知 -> WaitTask 第三次收到通知 -> 打印 take notification -> 继续等待下一次通知
```

横向总链路：

```text
WaitTask 等通知 -> NotifyTask 延时 1000ms -> NotifyTask 发通知 -> WaitTask 被唤醒 -> WaitTask 取走通知并清零 -> WaitTask 执行动作 -> WaitTask 再次等通知 -> 下一轮
```

### 如何理解实际日志

实际串口现象：

```text
lesson-21-task-notification start
[tick 1000] NotifyTask: give notification
[tick 1003] WaitTask: take notification
[tick 2007] NotifyTask: give notification
[tick 2010] WaitTask: take notification
[tick 3014] NotifyTask: give notification
[tick 3017] WaitTask: take notification
```

一组日志表示一次通知完成：

```text
NotifyTask: give notification -> WaitTask: take notification
```

`WaitTask` 比 `NotifyTask` 晚几 tick，是因为当前代码先打印 `NotifyTask` 日志，再真正发送通知：

```c
SendTaskLog("NotifyTask", "give notification");
xTaskNotifyGive(wait_task_handle);
```

串口打印是阻塞式发送，打印本身会消耗一点时间。所以 `WaitTask` 的日志会比 `NotifyTask` 晚几个 tick。

另外，`NotifyTask` 的时间点不是严格的 `1000、2000、3000`，而是：

```text
1000、2007、3014、4021...
```

原因是 `NotifyTask` 使用的是：

```c
vTaskDelay(pdMS_TO_TICKS(1000));
```

`vTaskDelay()` 是从任务执行到这句代码的时刻开始延时。任务每轮还要打印日志、发送通知，所以周期会变成：

```text
1000ms 延时 + 本轮任务自身执行时间
```

如果希望任务严格按绝对周期运行，应使用：

```c
vTaskDelayUntil()
```

本课重点不是严格周期，而是验证：

```text
NotifyTask 发通知 -> WaitTask 被唤醒 -> WaitTask 取走通知
```

## 本课结论

这一课要建立四个判断：

```text
1. 任务通知是直接发给某一个任务的同步机制。
2. xTaskNotifyGive() 用来给目标任务发送通知。
3. ulTaskNotifyTake() 用来等待并获取通知。
4. 一对一同步场景下，任务通知可以替代二值信号量，并且更轻量。
```

一句话：

```text
如果只是通知某一个任务继续运行，优先考虑任务通知。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd '/home/yang/stm32-FreeRTOS/lessons/lesson-21-task-notification/25-FreeRTOS任务通知'
make clean
make
```

生成产物：

```text
build/lesson-21-task-notification.elf
build/lesson-21-task-notification.hex
build/lesson-21-task-notification.bin
build/lesson-21-task-notification.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
