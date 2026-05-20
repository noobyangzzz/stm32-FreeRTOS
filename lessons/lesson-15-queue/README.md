# Lesson 15：队列 Queue

## 本课目标

前面的课程主要在讲任务自身：

```text
创建任务、任务参数、任务句柄、挂起、恢复、删除。
```

这一课开始进入任务间通信。FreeRTOS 队列用于在任务之间传递数据：

```text
ProducerTask -> xQueueSend() -> Queue -> xQueueReceive() -> ConsumerTask
```

本课重点：

```text
xQueueCreate()：创建队列。
xQueueSend()：向队列发送消息。
xQueueReceive()：从队列接收消息。
```

## 对比上一个 lesson

对比 Lesson 14，本课不再演示删除任务。

本课变化：

```text
User/main.c 新增 QueueMessage_t 消息结构体。
User/main.c 新增 message_queue 队列句柄。
User/main.c 新增 ProducerTask()。
User/main.c 新增 ConsumerTask()。
User/main.c 新增 CheckQueueCreated()。
Makefile 目标名改为 lesson-15-queue。
```

这部分实现的功能：

```text
ProducerTask：每 1000ms 构造一条消息，并通过队列发送。
ConsumerTask：阻塞等待队列消息，收到后通过串口打印。
QueueMessage_t：定义队列中每条消息的数据格式。
message_queue：保存队列句柄，供发送和接收 API 使用。
```

## 队列消息结构

本课定义的队列消息：

```c
typedef struct
{
  uint32_t sequence;
  uint32_t producer_tick;
} QueueMessage_t;
```

含义：

```text
sequence：消息序号，每发送一次递增。
producer_tick：生产者发送消息时的 tick。
```

队列里传的不是字符串，而是一份固定大小的结构体数据。

## xQueueCreate()

代码：

```c
message_queue = xQueueCreate(4, sizeof(QueueMessage_t));
CheckQueueCreated(message_queue);
```

横向理解：

```text
创建队列 -> 队列最多容纳 4 条消息 -> 每条消息大小是 sizeof(QueueMessage_t) -> 返回队列句柄 -> 检查是否创建成功
```

如果队列创建失败，通常是 FreeRTOS heap 不够。

## ProducerTask

完整函数：

```c
static void ProducerTask(void *argument)
{
  QueueMessage_t message;
  uint32_t sequence = 0;

  (void)argument;

  while (1)
  {
    message.sequence = sequence++;
    message.producer_tick = (uint32_t)xTaskGetTickCount();

    if (xQueueSend(message_queue, &message, pdMS_TO_TICKS(100)) == pdPASS)
    {
      LED3_TOGGLE;
      SendTaskLog("ProducerTask", "send message");
    }
    else
    {
      SendTaskLog("ProducerTask", "queue full, send failed");
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
```

带注释理解：

```c
static void ProducerTask(void *argument)
{
  // 定义一条队列消息。后面会把这条消息发送到 message_queue。
  QueueMessage_t message;

  // sequence 是生产者自己的消息序号。
  // 每成功构造一条消息，就让它递增。
  uint32_t sequence = 0;

  // 当前任务没有使用 xTaskCreate() 的第 4 个参数。
  // 写这句只是明确告诉编译器：argument 暂时不用。
  (void)argument;

  while (1)
  {
    // 填写消息序号。
    // sequence++ 的含义是：先使用当前 sequence，再让 sequence 加 1。
    // 所以第一次发送 seq=0，第二次发送 seq=1，依次递增。
    message.sequence = sequence++;

    // 记录生产者构造这条消息时的系统 tick。
    // 后面 ConsumerTask 打印 producer_tick，就能看到消息是什么时候被生产的。
    message.producer_tick = (uint32_t)xTaskGetTickCount();

    // 把 message 发送到 message_queue。
    // &message 表示传入 message 的地址。
    // 但队列内部会复制 message 的内容，不是只保存这个局部变量地址。
    // 第三个参数 100ms 表示：如果队列满了，最多等待 100ms。
    if (xQueueSend(message_queue, &message, pdMS_TO_TICKS(100)) == pdPASS)
    {
      // 发送成功后翻转蓝灯，作为发送动作的板上现象。
      LED3_TOGGLE;

      // 串口打印生产者已经发送消息。
      SendTaskLog("ProducerTask", "send message");
    }
    else
    {
      // 如果 100ms 内队列仍然满，就发送失败。
      // 当前实验里 ConsumerTask 接收很快，正常不会走到这里。
      SendTaskLog("ProducerTask", "queue full, send failed");
    }

    // 生产者发送完一条消息后阻塞 1000ms。
    // 这不是裸机空转，而是让出 CPU，等 1000 tick 后再重新就绪。
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
```

这个函数的横向链路：

```text
ProducerTask 运行 -> 填写 sequence -> 填写 producer_tick -> xQueueSend() 发送到队列 -> 发送成功则翻转 LED3 并打印 send message -> vTaskDelay(1000ms)
```

这段代码里最重要的一点是：

```text
xQueueSend() 发送的是 message 的内容副本。
```

所以 `message` 虽然是 `ProducerTask()` 里的局部变量，但不会因为函数继续循环而导致队列里的数据失效。

## ConsumerTask

完整函数：

```c
static void ConsumerTask(void *argument)
{
  QueueMessage_t message;

  (void)argument;

  while (1)
  {
    if (xQueueReceive(message_queue, &message, portMAX_DELAY) == pdPASS)
    {
      SendQueueMessageLog(&message);
    }
  }
}
```

带注释理解：

```c
static void ConsumerTask(void *argument)
{
  // 定义一个本地变量，用来接收队列中取出的消息。
  QueueMessage_t message;

  // 当前任务没有使用 xTaskCreate() 的第 4 个参数。
  (void)argument;

  while (1)
  {
    // 从 message_queue 接收一条消息，并复制到本地变量 message 中。
    // &message 是接收缓冲区地址。
    // portMAX_DELAY 表示：如果队列为空，就一直阻塞等待。
    // 所以 ConsumerTask 没有消息时不会空转占 CPU。
    if (xQueueReceive(message_queue, &message, portMAX_DELAY) == pdPASS)
    {
      // 成功收到消息后，把消息内容打印出来。
      // 这里能看到 seq 和 producer_tick。
      SendQueueMessageLog(&message);
    }
  }
}
```

这个函数的横向链路：

```text
ConsumerTask 运行 -> xQueueReceive(message_queue, &message, portMAX_DELAY) -> 没消息就阻塞等待 -> 收到消息后打印 sequence 和 producer_tick -> 继续等待下一条消息
```

这里最重要的是：

```text
xQueueReceive() 不只是取数据，也会让 ConsumerTask 在没数据时阻塞。
```

所以队列同时解决两个问题：

```text
1. 数据传递：ProducerTask 把 QueueMessage_t 传给 ConsumerTask。
2. 任务同步：ConsumerTask 没消息时阻塞，有消息时被唤醒。
```

这就是为什么队列是 FreeRTOS 任务间通信的基础机制。

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印 lesson-15-queue start -> xQueueCreate() -> CheckQueueCreated() -> CreateTasks() -> vTaskStartScheduler()
```

`CreateTasks()` 的横向流程：

```text
创建 ProducerTask -> 检查创建结果 -> 创建 ConsumerTask -> 检查创建结果
```

## 预期串口现象

Reset 后先输出：

```text
lesson-15-queue start
```

随后可以看到类似：

```text
[tick 0] ProducerTask: send message
[tick 3] ConsumerTask: receive seq=0, producer_tick=0
[tick 1004] ProducerTask: send message
[tick 1007] ConsumerTask: receive seq=1, producer_tick=1004
[tick 2008] ProducerTask: send message
[tick 2011] ConsumerTask: receive seq=2, producer_tick=2008
```

重点观察：

```text
ProducerTask 每次发送后，ConsumerTask 很快收到。
seq 每次递增。
producer_tick 是发送消息时记录的 tick。
ConsumerTask 的 receive tick 通常略晚于 producer_tick，因为接收和串口打印需要调度和执行时间。
```

## 为什么要用队列

不用队列时，任务之间如果直接共享变量，容易遇到：

```text
读写竞争。
数据被覆盖。
不知道新数据什么时候到。
需要自己设计同步逻辑。
```

用队列后：

```text
发送方只负责 xQueueSend()。
接收方只负责 xQueueReceive()。
队列负责保存消息、阻塞等待、唤醒接收任务。
```

一句话：

```text
队列把“传数据”和“任务同步”合在了一起。
```

## 本课结论

这一课要建立四个判断：

```text
1. Queue 是任务间传递数据的 FreeRTOS 对象。
2. xQueueSend() 会把消息内容复制进队列。
3. xQueueReceive() 可以阻塞等待消息，避免任务空转。
4. 队列既能传数据，也能让接收任务在有数据时被唤醒。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd '/home/yang/stm32-FreeRTOS/lessons/lesson-15-queue/19-FreeRTOS队列'
make clean
make
```

生成产物：

```text
build/lesson-15-queue.elf
build/lesson-15-queue.hex
build/lesson-15-queue.bin
build/lesson-15-queue.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
