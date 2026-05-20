# Lesson 19：事件组 Event Group

## 本课目标

前面几课分别讲了：

```text
Queue：传具体数据。
Binary Semaphore：通知一个事件。
Counting Semaphore：累计同类事件数量。
Mutex：保护共享资源。
```

这一课讲事件组：

```text
Event Group 用多个 bit 表示多个不同事件。
```

本课目标是观察：

```text
EventTaskA 设置 bit A。
EventTaskB 设置 bit B。
WaitTask 等 bit A 和 bit B 都到齐后继续运行。
```

## 对比上一个 lesson

对比 Lesson 18，本课不再使用 mutex。

本课变化：

```text
User/main.c 引入 event_groups.h。
User/main.c 新增 EVENT_BIT_A 和 EVENT_BIT_B。
User/main.c 新增 sync_event_group 事件组句柄。
User/main.c 新增 EventTaskA()、EventTaskB() 和 WaitTask()。
Makefile 目标名改为 lesson-19-event-group。
```

这部分实现的功能：

```text
EventTaskA：每 1000ms 设置 EVENT_BIT_A。
EventTaskB：每 1500ms 设置 EVENT_BIT_B。
WaitTask：等待 EVENT_BIT_A 和 EVENT_BIT_B 同时满足。
```

## 事件位

本课定义：

```c
#define EVENT_BIT_A ( ( EventBits_t ) 0x01 )
#define EVENT_BIT_B ( ( EventBits_t ) 0x02 )
```

二进制理解：

```text
EVENT_BIT_A = 0x01 = 0000 0001
EVENT_BIT_B = 0x02 = 0000 0010
```

两个事件都到齐时：

```text
EVENT_BIT_A | EVENT_BIT_B = 0x03 = 0000 0011
```

## xEventGroupCreate()

代码：

```c
sync_event_group = xEventGroupCreate();
CheckEventGroupCreated(sync_event_group);
```

横向理解：

```text
创建事件组 -> 返回事件组句柄 -> 检查是否创建成功
```

## EventTaskA

完整函数：

```c
static void EventTaskA(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
    SendTaskLog("EventTaskA", "set bit A");
    xEventGroupSetBits(sync_event_group, EVENT_BIT_A);
  }
}
```

横向流程：

```text
EventTaskA 运行 -> 延时 1000ms -> 打印 set bit A -> 设置 EVENT_BIT_A -> 重复
```

## EventTaskB

完整函数：

```c
static void EventTaskB(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1500));
    SendTaskLog("EventTaskB", "set bit B");
    xEventGroupSetBits(sync_event_group, EVENT_BIT_B);
  }
}
```

横向流程：

```text
EventTaskB 运行 -> 延时 1500ms -> 打印 set bit B -> 设置 EVENT_BIT_B -> 重复
```

## WaitTask

完整函数：

```c
static void WaitTask(void *argument)
{
  EventBits_t bits;

  (void)argument;

  while (1)
  {
    bits = xEventGroupWaitBits(sync_event_group,
                               EVENT_BIT_A | EVENT_BIT_B,
                               pdTRUE,
                               pdTRUE,
                               portMAX_DELAY);

    LED3_TOGGLE;
    SendEventBitsLog("WaitTask", "bit A and bit B ready", bits);
  }
}
```

参数指南：

```c
bits = xEventGroupWaitBits(
    sync_event_group,          /* 等待哪个事件组 */
    EVENT_BIT_A | EVENT_BIT_B, /* 等待哪些 bit；这里表示同时关心 bit A 和 bit B */
    pdTRUE,                    /* 返回前是否清除已满足的 bit；pdTRUE 表示自动清除 */
    pdTRUE,                    /* 是否要求所有 bit 都满足；pdTRUE 表示 A 和 B 都要到齐 */
    portMAX_DELAY              /* 最长等待时间；portMAX_DELAY 表示一直阻塞等待 */
);
```

参数含义：

```text
sync_event_group：等待哪个事件组。
EVENT_BIT_A | EVENT_BIT_B：等待 bit A 和 bit B。
pdTRUE：等待成功后自动清除这些 bit。
pdTRUE：要求所有 bit 都满足，也就是 A 和 B 都到齐。
portMAX_DELAY：一直阻塞等待，直到条件满足。
```

所以 WaitTask 的横向流程：

```text
WaitTask 运行 -> 等待 bit A 和 bit B 都置位 -> 条件满足后返回 -> 翻转 LED3 -> 打印 bits -> 自动清除 bit A/bit B -> 继续下一轮等待
```

## `|` 和 `&` 的区别

这里必须写：

```c
EVENT_BIT_A | EVENT_BIT_B
```

原因是：

```text
EVENT_BIT_A = 0x01 = 0000 0001
EVENT_BIT_B = 0x02 = 0000 0010
```

用 `|` 合并后：

```text
0000 0001
0000 0010
---------
0000 0011 = 0x03
```

也就是告诉 FreeRTOS：

```text
我要等待 bit A 和 bit B 这两个事件位。
```

如果写成：

```c
EVENT_BIT_A & EVENT_BIT_B
```

结果会是：

```text
0000 0001
0000 0010
---------
0000 0000 = 0x00
```

这就变成“没有等待任何有效 bit”，语义是错的。

经验规则：

```text
组合多个事件位时，用 |。
检查某个事件位是否存在时，常用 &。
```

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印 lesson-19-event-group start -> xEventGroupCreate() -> CheckEventGroupCreated() -> CreateTasks() -> vTaskStartScheduler()
```

`CreateTasks()` 的横向流程：

```text
创建 EventTaskA -> 创建 EventTaskB -> 创建 WaitTask
```

## 预期串口现象

Reset 后先输出：

```text
lesson-19-event-group start
```

随后可以看到类似：

```text
[tick 1000] EventTaskA: set bit A
[tick 1500] EventTaskB: set bit B
[tick 1503] WaitTask: bit A and bit B ready, bits=0x3
[tick 2004] EventTaskA: set bit A
[tick 3007] EventTaskB: set bit B
[tick 3010] WaitTask: bit A and bit B ready, bits=0x3
```

重点观察：

```text
WaitTask 不会在只有 bit A 时运行。
WaitTask 也不会在只有 bit B 时运行。
只有 bit A 和 bit B 都到齐，WaitTask 才继续。
WaitTask 返回后自动清除 bit A 和 bit B，进入下一轮等待。
```

## 实际现象分析

你观察到的日志：

```text
lesson-19-event-group start
[tick 1000] EventTaskA: set bit A
[tick 1500] EventTaskB: set bit B
[tick 1503] WaitTask: bit A and bit B ready, bits=0x3
[tick 2003] EventTaskA: set bit A
[tick 3006] EventTaskA: set bit A
[tick 3009] EventTaskB: set bit B
[tick 3012] WaitTask: bit A and bit B ready, bits=0x3
```

横向理解：

```text
EventTaskA 每约 1000ms 设置 bit A -> EventTaskB 每约 1500ms 设置 bit B -> WaitTask 等 A/B 都到齐 -> 输出 bits=0x3 -> 自动清除 A/B -> 进入下一轮等待
```

其中：

```text
bits=0x3 表示 EVENT_BIT_A 和 EVENT_BIT_B 都已经置位。
```

注意 `[tick 2003]` 和 `[tick 3006]` 连续出现两次 `EventTaskA: set bit A`，但 WaitTask 没有在 2003 运行。原因是：

```text
bit A 已经到了，但 bit B 还没到。
WaitTask 设置的是等待全部 bit，也就是 A 和 B 都满足才返回。
```

另外，bit 不是计数器。A 连续设置两次，结果仍然只是：

```text
bit A = 1
```

它不会变成“A 有两个”。如果需要累计次数，应使用计数信号量或队列，而不是事件组。

## Event Group 适用场景

事件组适合表示多个条件组合，例如：

```text
网络已连接 + 时间已同步。
传感器 A 就绪 + 传感器 B 就绪。
按键事件 + 串口命令事件。
多个初始化步骤全部完成。
多个任务都到达某个同步点。
```

一句话：

```text
多个不同事件，用 bit 表示；等待某一个或某几个 bit 满足，用 Event Group。
```

## 本课结论

这一课要建立四个判断：

```text
1. Event Group 用 bit 表示多个不同事件。
2. xEventGroupSetBits() 用来设置事件位。
3. xEventGroupWaitBits() 可以等待一个或多个事件位。
4. Event Group 适合处理多个条件组合，而不是只处理单一事件。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd '/home/yang/stm32-FreeRTOS/lessons/lesson-19-event-group/23-FreeRTOS事件组'
make clean
make
```

生成产物：

```text
build/lesson-19-event-group.elf
build/lesson-19-event-group.hex
build/lesson-19-event-group.bin
build/lesson-19-event-group.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
