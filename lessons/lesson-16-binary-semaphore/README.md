# Lesson 16：二值信号量 Binary Semaphore

## 本课目标

上一课讲队列：

```text
ProducerTask -> xQueueSend() -> Queue -> xQueueReceive() -> ConsumerTask
```

队列适合传递具体数据，例如 `seq` 和 `producer_tick`。

这一课讲二值信号量：

```text
SignalTask -> xSemaphoreGive() -> Binary Semaphore -> xSemaphoreTake() -> WaitTask
```

二值信号量通常不关心具体数据，只表达：

```text
某个事件发生了。
```

## 对比上一个 lesson

对比 Lesson 15，本课不再传结构体消息。

本课变化：

```text
User/main.c 删除 QueueMessage_t。
User/main.c 删除 ProducerTask() 和 ConsumerTask()。
User/main.c 新增 event_semaphore 二值信号量句柄。
User/main.c 新增 SignalTask()。
User/main.c 新增 WaitTask()。
User/main.c 新增 CheckSemaphoreCreated()。
Makefile 目标名改为 lesson-16-binary-semaphore。
```

这部分实现的功能：

```text
SignalTask：每 1000ms give 一次二值信号量，表示事件发生。
WaitTask：阻塞等待二值信号量，take 成功后翻转 LED3 并打印日志。
event_semaphore：保存二值信号量句柄，供 give/take 使用。
```

## Queue 和 Binary Semaphore 的区别

```text
Queue：传数据。接收方能拿到发送方放进去的具体内容。
Binary Semaphore：发通知。接收方只知道事件发生了，不拿具体数据。
```

可以这样理解：

```text
Queue 像消息箱：里面有具体信件。
Binary Semaphore 像门铃：只告诉你有人按了，不携带复杂内容。
```

## xSemaphoreCreateBinary()

代码：

```c
event_semaphore = xSemaphoreCreateBinary();
CheckSemaphoreCreated(event_semaphore);
```

横向理解：

```text
创建二值信号量 -> 返回信号量句柄 -> 检查是否创建成功
```

二值信号量只有两种状态：

```text
没有信号：WaitTask take 时会阻塞。
有信号：WaitTask take 成功，然后信号被消耗掉。
```

## SignalTask

完整函数：

```c
static void SignalTask(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));

    if (xSemaphoreGive(event_semaphore) == pdPASS)
    {
      SendTaskLog("SignalTask", "give semaphore");
    }
    else
    {
      SendTaskLog("SignalTask", "semaphore already available");
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
    // 每 1000ms 产生一次事件。
    // 这里用延时模拟“事件周期性发生”。
    vTaskDelay(pdMS_TO_TICKS(1000));

    // give 二值信号量，表示事件发生。
    // 如果 WaitTask 正在阻塞等待，它会被唤醒。
    if (xSemaphoreGive(event_semaphore) == pdPASS)
    {
      SendTaskLog("SignalTask", "give semaphore");
    }
    else
    {
      // 二值信号量最多只能保存一个“有信号”状态。
      // 如果之前的信号还没被 take 掉，再 give 就可能失败。
      SendTaskLog("SignalTask", "semaphore already available");
    }
  }
}
```

这个函数的横向链路：

```text
SignalTask 运行 -> vTaskDelay(1000ms) -> xSemaphoreGive(event_semaphore) -> 打印 give semaphore -> 重复
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
      SendTaskLog("WaitTask", "take semaphore");
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
    // 等待二值信号量。
    // 如果当前没有信号，WaitTask 会阻塞，不会空转占 CPU。
    // portMAX_DELAY 表示一直等到有信号为止。
    if (xSemaphoreTake(event_semaphore, portMAX_DELAY) == pdPASS)
    {
      // take 成功表示事件已经到来。
      // 同时这个信号会被消耗掉。
      LED3_TOGGLE;
      SendTaskLog("WaitTask", "take semaphore");
    }
  }
}
```

这个函数的横向链路：

```text
WaitTask 运行 -> xSemaphoreTake(event_semaphore, portMAX_DELAY) -> 没信号就阻塞 -> 有信号后被唤醒 -> 翻转 LED3 -> 打印 take semaphore -> 继续等待
```

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印 lesson-16-binary-semaphore start -> xSemaphoreCreateBinary() -> CheckSemaphoreCreated() -> CreateTasks() -> vTaskStartScheduler()
```

`CreateTasks()` 的横向流程：

```text
创建 SignalTask -> 检查创建结果 -> 创建 WaitTask -> 检查创建结果
```

## 预期串口现象

Reset 后先输出：

```text
lesson-16-binary-semaphore start
```

随后可以看到类似：

```text
[tick 1000] SignalTask: give semaphore
[tick 1003] WaitTask: take semaphore
[tick 2004] SignalTask: give semaphore
[tick 2007] WaitTask: take semaphore
[tick 3008] SignalTask: give semaphore
[tick 3011] WaitTask: take semaphore
```

重点观察：

```text
SignalTask give 后，WaitTask 很快 take。
WaitTask 没有信号时不输出，说明它在阻塞等待。
日志中没有 seq、producer_tick 这样的数据，因为二值信号量只表示事件。
```

## 实际现象分析

实际烧录后，可能看到类似日志：

```text
lesson-16-binary-semaphore start
[tick 1000] SignalTask: give semaphore
[tick 1003] WaitTask: take semaphore
[tick 2006] SignalTask: give semaphore
[tick 2009] WaitTask: take semaphore
[tick 3012] SignalTask: give semaphore
[tick 3015] WaitTask: take semaphore
[tick 4018] SignalTask: give semaphore
[tick 4021] WaitTask: take semaphore
```

这个现象是正确的。每两行是一组完整事件：

```text
SignalTask give -> 二值信号量从 0 变成 1 -> WaitTask 被唤醒 -> WaitTask take -> 二值信号量从 1 变回 0
```

例如：

```text
[tick 4018] SignalTask: give semaphore
[tick 4021] WaitTask: take semaphore
```

含义是：

```text
SignalTask 在 tick 4018 释放二值信号量。
WaitTask 在 tick 4021 获取并消费这个信号量。
```

两者通常相差几 tick，这是正常的，原因包括：

```text
SignalTask 串口打印耗时。
FreeRTOS 调度切换耗时。
WaitTask 被唤醒后执行串口打印耗时。
```

`SignalTask` 的日志间隔也通常略大于 1000 tick，例如：

```text
1000 -> 2006 -> 3012 -> 4018
```

原因是每一轮除了 `vTaskDelay(1000ms)`，还执行了：

```text
xSemaphoreGive()
串口打印
任务切换
```

所以实际看到约 1006 tick 是正常现象。

这份日志验证了本课核心链路：

```text
WaitTask 平时阻塞在 xSemaphoreTake(event_semaphore, portMAX_DELAY)
-> SignalTask 周期性 xSemaphoreGive(event_semaphore)
-> WaitTask 被唤醒
-> WaitTask take 成功并打印日志
-> WaitTask 继续回到阻塞等待
```

一句话：

```text
SignalTask 周期性释放事件，WaitTask 阻塞等待事件；事件发生后，WaitTask 很快被唤醒并消费这个二值信号量。
```

## 课堂讨论记录

### xSemaphoreGive() 和 xSemaphoreTake() 是否可以理解成 +1 / -1

可以先这样理解，但要加上一个前提：

```text
本课使用的是二值信号量，它的状态只有 0 和 1。
```

所以：

```text
xSemaphoreGive()：尝试把信号量从 0 变成 1。
xSemaphoreTake()：尝试把信号量从 1 变成 0。
```

也可以类比成：

```text
Give：释放一个事件。
Take：取走并消费这个事件。
```

但是二值信号量不能无限累加：

```text
0 -> give -> 1
1 -> give -> 仍然最多是 1，通常返回 pdFAIL
1 -> take -> 0
0 -> take -> 没有信号，任务可以阻塞等待
```

在本课代码中：

```c
xSemaphoreGive(event_semaphore);
```

表示：

```text
SignalTask 发出一个事件。
```

而：

```c
xSemaphoreTake(event_semaphore, portMAX_DELAY);
```

表示：

```text
WaitTask 等待这个事件；如果事件还没发生，就阻塞；如果事件已经发生，就取走它。
```

### SemaphoreHandle_t 是什么

代码：

```c
static SemaphoreHandle_t event_semaphore = NULL;
```

`SemaphoreHandle_t` 是信号量句柄，可以理解成：

```text
应用层用来引用某个信号量对象的变量。
```

创建信号量：

```c
event_semaphore = xSemaphoreCreateBinary();
```

这句话做了两件事：

```text
FreeRTOS 在 heap 中创建一个二值信号量对象。
返回一个句柄，保存到 event_semaphore。
```

之后操作这个信号量，都要通过这个句柄：

```c
xSemaphoreGive(event_semaphore);
xSemaphoreTake(event_semaphore, portMAX_DELAY);
```

所以：

```text
SemaphoreHandle_t 不是信号量本体；
它是指向/引用信号量本体的句柄。
```

类比之前学过的任务句柄：

```text
TaskHandle_t：用来引用某个任务。
SemaphoreHandle_t：用来引用某个信号量。
```

### 二值信号量会不会超过 1

本课中的信号量不会超过 1。

原因不是代码手动限制了它，而是因为创建方式决定了它是二值信号量：

```c
event_semaphore = xSemaphoreCreateBinary();
```

这个 API 创建出来的信号量最大计数值就是 1，所以它天然只有两种状态：

```text
0：没有信号。
1：有信号。
```

如果它已经是 1，再调用：

```c
xSemaphoreGive(event_semaphore);
```

不会让它变成 2。通常结果是：

```text
give 失败，返回 pdFAIL。
```

所以本课代码中有这个分支：

```c
if (xSemaphoreGive(event_semaphore) == pdPASS)
{
  SendTaskLog("SignalTask", "give semaphore");
}
else
{
  SendTaskLog("SignalTask", "semaphore already available");
}
```

`semaphore already available` 的意思是：

```text
信号量已经是 1，还没有被 WaitTask take 掉；
这时再次 give，不能继续增加，所以 give 失败。
```

当前实验里，WaitTask 一直在阻塞等待：

```c
xSemaphoreTake(event_semaphore, portMAX_DELAY);
```

所以 SignalTask 每次 give 后，WaitTask 很快 take 掉。正常情况下会看到：

```text
SignalTask: give semaphore
WaitTask: take semaphore
```

不太会看到：

```text
SignalTask: semaphore already available
```

如果想让信号量可以累计到 2、3、4，就不是二值信号量，而是计数信号量：

```c
xSemaphoreCreateCounting(max_count, initial_count);
```

例如：

```c
xSemaphoreCreateCounting(5, 0);
```

表示：

```text
最小值 0，最大值 5。
```

这时多次 give 可以让计数变化：

```text
0 -> 1 -> 2 -> 3 -> 4 -> 5
```

再 give 就失败。

一句话总结：

```text
二值信号量：最大值固定是 1，只表示有没有事件。
计数信号量：最大值可以设置成 N，可以累计多个事件或资源数量。
```

## 本课结论

这一课要建立四个判断：

```text
1. 二值信号量适合表达“事件发生了”。
2. xSemaphoreGive() 释放信号量，可以唤醒等待任务。
3. xSemaphoreTake() 获取信号量；没有信号时可以阻塞等待。
4. 二值信号量不传递复杂数据，只做事件同步。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd '/home/yang/stm32-FreeRTOS/lessons/lesson-16-binary-semaphore/20-FreeRTOS二值信号量'
make clean
make
```

生成产物：

```text
build/lesson-16-binary-semaphore.elf
build/lesson-16-binary-semaphore.hex
build/lesson-16-binary-semaphore.bin
build/lesson-16-binary-semaphore.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
