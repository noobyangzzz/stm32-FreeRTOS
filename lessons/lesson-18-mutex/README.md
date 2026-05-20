# Lesson 18：互斥量 Mutex

## 本课目标

上一课讲计数信号量：

```text
计数信号量可以表示 N 个事件或 N 个同类资源。
```

这一课讲互斥量：

```text
Mutex 用来保护共享资源，同一时间只允许一个任务持有。
```

本课重点：

```text
xSemaphoreCreateMutex()
xSemaphoreTake()
xSemaphoreGive()
共享资源互斥访问
优先级继承
```

## 对比上一个 lesson

对比 Lesson 17，本课不再观察 count 累积。

本课变化：

```text
User/FreeRTOSConfig.h 将 configUSE_MUTEXES 从 0 改为 1。
User/FreeRTOSConfig.h 将 configUSE_COUNTING_SEMAPHORES 改回 0。
User/main.c 将 xSemaphoreCreateCounting() 改为 xSemaphoreCreateMutex()。
User/main.c 新增 LowTask() 和 HighTask()。
User/main.c 使用 shared_resource_mutex 保护共享资源。
Makefile 目标名改为 lesson-18-mutex。
```

这部分实现的功能：

```text
LowTask：低优先级任务，先拿到 mutex，并模拟长时间使用共享资源。
HighTask：高优先级任务，稍后尝试拿同一个 mutex，必须等待 LowTask 释放。
shared_resource_mutex：保护共享资源的互斥量。
```

## Mutex 和 Counting Semaphore 的区别

```text
Counting Semaphore：关注数量，可以允许 N 个资源或事件累计。
Mutex：关注所有权，同一时间只能一个任务持有。
```

可以这样理解：

```text
计数信号量 = 有几个名额。
互斥量 = 这把锁现在谁拿着。
```

Mutex 更适合保护共享资源，例如：

```text
共享串口输出。
共享变量。
共享外设。
共享缓冲区。
```

## FreeRTOSConfig.h 关键配置

要使用 mutex，需要打开：

```c
#define configUSE_MUTEXES 1
```

FreeRTOS mutex 支持优先级继承。简单理解：

```text
如果低优先级任务持有 mutex，而高优先级任务正在等待这个 mutex，
FreeRTOS 可以临时提高低优先级任务的有效优先级，
让它尽快运行并释放 mutex，
从而缓解优先级反转。
```

## xSemaphoreCreateMutex()

代码：

```c
shared_resource_mutex = xSemaphoreCreateMutex();
CheckMutexCreated(shared_resource_mutex);
```

横向理解：

```text
创建 mutex -> 返回 mutex 句柄 -> 检查是否创建成功
```

Mutex 创建后通常是可用状态：

```text
第一个 xSemaphoreTake() 可以成功拿到 mutex。
```

## LowTask

完整函数：

```c
static void LowTask(void *argument)
{
  (void)argument;

  while (1)
  {
    SendTaskLog("LowTask", "try take mutex");

    if (xSemaphoreTake(shared_resource_mutex, portMAX_DELAY) == pdPASS)
    {
      SendTaskLog("LowTask", "take mutex, use shared resource");
      vTaskDelay(pdMS_TO_TICKS(3000));
      SendTaskLog("LowTask", "give mutex");
      xSemaphoreGive(shared_resource_mutex);
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
```

带注释理解：

```c
static void LowTask(void *argument)
{
  // 当前任务没有使用 xTaskCreate() 第 4 个参数。
  (void)argument;

  while (1)
  {
    // LowTask 准备获取 mutex。
    SendTaskLog("LowTask", "try take mutex");

    // 尝试获取共享资源的 mutex。
    // 如果 mutex 被别人持有，就一直阻塞等待。
    if (xSemaphoreTake(shared_resource_mutex, portMAX_DELAY) == pdPASS)
    {
      // 获取成功后，LowTask 进入共享资源区域。
      SendTaskLog("LowTask", "take mutex, use shared resource");

      // 模拟低优先级任务长时间占用共享资源。
      vTaskDelay(pdMS_TO_TICKS(3000));

      // 使用完共享资源后释放 mutex。
      SendTaskLog("LowTask", "give mutex");
      xSemaphoreGive(shared_resource_mutex);
    }

    // 释放后等一段时间，再进入下一轮。
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
```

## HighTask

完整函数：

```c
static void HighTask(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
    SendTaskLog("HighTask", "try take mutex");

    if (xSemaphoreTake(shared_resource_mutex, portMAX_DELAY) == pdPASS)
    {
      LED3_TOGGLE;
      SendTaskLog("HighTask", "take mutex, use shared resource");
      vTaskDelay(pdMS_TO_TICKS(500));
      SendTaskLog("HighTask", "give mutex");
      xSemaphoreGive(shared_resource_mutex);
    }

    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}
```

带注释理解：

```c
static void HighTask(void *argument)
{
  // 当前任务没有使用 xTaskCreate() 第 4 个参数。
  (void)argument;

  while (1)
  {
    // 先延时 1000ms，让 LowTask 有机会先拿到 mutex。
    vTaskDelay(pdMS_TO_TICKS(1000));

    // HighTask 尝试获取同一个 mutex。
    SendTaskLog("HighTask", "try take mutex");

    // 如果 LowTask 正在持有 mutex，HighTask 会阻塞等待。
    if (xSemaphoreTake(shared_resource_mutex, portMAX_DELAY) == pdPASS)
    {
      // 等 LowTask 释放 mutex 后，HighTask 才能进入共享资源区域。
      LED3_TOGGLE;
      SendTaskLog("HighTask", "take mutex, use shared resource");

      // 模拟高优先级任务短时间使用共享资源。
      vTaskDelay(pdMS_TO_TICKS(500));

      // 使用完共享资源后释放 mutex。
      SendTaskLog("HighTask", "give mutex");
      xSemaphoreGive(shared_resource_mutex);
    }

    // 等一段时间，再开始下一轮。
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}
```

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印 lesson-18-mutex start -> xSemaphoreCreateMutex() -> CheckMutexCreated() -> CreateTasks() -> vTaskStartScheduler()
```

`CreateTasks()` 的横向流程：

```text
创建 LowTask，优先级 1 -> 创建 HighTask，优先级 3
```

任务竞争链路：

```text
LowTask 先 take mutex -> LowTask 使用共享资源 3000ms -> HighTask 延时 1000ms 后 try take mutex -> HighTask 等待 -> LowTask give mutex -> HighTask take mutex -> HighTask 使用共享资源 500ms -> HighTask give mutex
```

## 预期串口现象

Reset 后先输出：

```text
lesson-18-mutex start
```

随后可以看到类似：

```text
[tick 0] LowTask: try take mutex
[tick 3] LowTask: take mutex, use shared resource
[tick 1000] HighTask: try take mutex
[tick 3007] LowTask: give mutex
[tick 3010] HighTask: take mutex, use shared resource
[tick 3514] HighTask: give mutex
```

重点观察：

```text
LowTask 先拿到 mutex。
HighTask 虽然优先级更高，但 mutex 被 LowTask 持有，所以必须等待。
LowTask give mutex 后，HighTask 很快 take 成功。
同一时间只有一个任务进入共享资源区域。
```

## 实际现象分析

实际烧录后，可能看到类似日志：

```text
lesson-18-mutex start
[tick 0] LowTask: try take mutex
[tick 3] LowTask: take mutex, use shared resource
[tick 1000] HighTask: try take mutex
[tick 3007] LowTask: give mutex
[tick 3009] HighTask: take mutex, use shared resource
[tick 3514] HighTask: give mutex
[tick 5014] LowTask: try take mutex
[tick 5017] LowTask: take mutex, use shared resource
```

第一轮可以这样理解：

```text
LowTask 在 tick 0 尝试拿 mutex。
LowTask 在 tick 3 成功拿到 mutex，并进入共享资源区域。
HighTask 在 tick 1000 尝试拿同一个 mutex。
此时 mutex 仍然被 LowTask 持有，所以 HighTask 阻塞等待。
LowTask 在 tick 3007 释放 mutex。
HighTask 在 tick 3009 很快拿到 mutex，并进入共享资源区域。
HighTask 在 tick 3514 释放 mutex。
```

这说明：

```text
HighTask 优先级更高，但不能绕过 mutex。
mutex 控制共享资源访问权。
调度器优先级控制 CPU 运行权。
```

所以本课现象证明的是：

```text
LowTask 持锁时，HighTask 必须等待。
LowTask 释放锁后，HighTask 才能进入共享资源区域。
同一时间不会出现 LowTask 和 HighTask 同时使用共享资源。
```

### 当前实验没有展示什么

当前实验不是在展示：

```text
LowTask 和 HighTask 同一时刻都想拿 mutex，然后 HighTask 因为优先级高先拿到。
```

因为代码刻意安排：

```text
LowTask tick 0 先尝试拿 mutex。
HighTask 先 vTaskDelay(1000ms)，tick 1000 左右才尝试拿 mutex。
```

所以当前实验展示的是：

```text
LowTask 先持有 mutex -> HighTask 后来尝试 -> HighTask 阻塞等待 -> LowTask 释放 -> HighTask 继续。
```

如果要展示“同时竞争时高优先级先运行”，可以把两个任务设计成同一 tick 就绪。但那主要证明的是调度器优先级规则，不是 mutex 本身的互斥能力。

## 二值信号量也能做互斥吗

从表面效果看，二值信号量也可以做到：

```text
同一时间只让一个任务进入某段代码。
```

例如：

```text
进入共享资源前：xSemaphoreTake(binary_semaphore, portMAX_DELAY)
离开共享资源后：xSemaphoreGive(binary_semaphore)
```

但二值信号量和 mutex 的设计目的不同。

二值信号量更像事件通知：

```text
A give，B take。
```

例如：

```text
中断 give，任务 take。
SignalTask give，WaitTask take。
```

这表示某个事件发生了。

Mutex 更像资源锁：

```text
谁 take，谁 give。
```

例如：

```text
TaskA take mutex -> 访问共享资源 -> TaskA give mutex。
```

Mutex 强调所有权，并支持优先级继承；二值信号量没有 mutex 这种所有权语义，也没有优先级继承机制。

### 适用场景

二值信号量适合：

```text
任务等待某个事件发生。
中断通知任务。
一个任务通知另一个任务可以继续。
按键事件通知。
串口接收完成通知。
DMA 传输完成通知。
传感器数据就绪通知。
```

典型模式：

```text
事件产生方：xSemaphoreGive()
事件处理方：xSemaphoreTake()
```

互斥量适合：

```text
保护共享资源。
保护共享变量。
保护串口打印。
保护 I2C/SPI 总线。
保护 LCD 显示缓冲区。
保护文件系统访问。
保护多个任务都会访问的全局结构体。
```

典型模式：

```text
任务：take mutex -> 访问共享资源 -> give mutex
```

一句话区分：

```text
有事叫醒任务，用二值信号量。
多人抢同一个资源，用互斥量。
```

## 本课结论

这一课要建立四个判断：

```text
1. Mutex 用来保护共享资源。
2. xSemaphoreTake(mutex) 表示拿锁；拿不到时任务可以阻塞等待。
3. xSemaphoreGive(mutex) 表示释放锁。
4. Mutex 强调所有权和互斥访问；它不是用来累计事件数量的。
5. 二值信号量也能做简单互斥效果，但更适合事件通知；保护共享资源优先使用 mutex。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd '/home/yang/stm32-FreeRTOS/lessons/lesson-18-mutex/22-FreeRTOS互斥量'
make clean
make
```

生成产物：

```text
build/lesson-18-mutex.elf
build/lesson-18-mutex.hex
build/lesson-18-mutex.bin
build/lesson-18-mutex.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
