# Lesson 22：中断中使用任务通知 FromISR

## 背景：为什么要把中断和任务结合起来

在真实嵌入式系统里，很多事件不是程序主动轮询出来的，而是外设突然发生的。

例如：

```text
用户按下按键。
串口收到一个字节。
DMA 搬运完一块数据。
ADC 完成一次采样。
定时器捕获到一个输入脉冲。
传感器 INT 引脚提示有新数据。
```

这些事件有一个共同特点：

```text
它们发生的时间不由主程序决定。
```

如果只靠任务轮询，任务就要不断检查外设状态：

```text
任务运行 -> 检查按键/串口/DMA/ADC -> 没有事件 -> 继续检查 -> 继续占用 CPU
```

这种方式的问题是：

```text
响应可能不及时。
CPU 时间被浪费在反复检查上。
检查频率越高，CPU 浪费越多。
检查频率越低，事件响应越慢。
```

所以硬件提供了中断机制：

```text
外设事件发生 -> CPU 暂停当前任务 -> 进入中断服务函数 -> 处理事件 -> 返回原来的执行流
```

中断的优势是：

```text
响应快。
不需要任务一直轮询。
事件发生时 CPU 才处理。
```

但如果所有业务都直接放在中断里，也会有问题。

例如在中断里直接做这些事：

```text
串口打印大量日志。
执行复杂状态机。
做较长时间的数据处理。
等待某个资源。
访问可能被其他任务共享的外设。
```

问题是：

```text
中断会抢占任务，中断执行太久会拖慢整个系统。
中断里不能阻塞等待。
中断里不能随便调用普通 FreeRTOS API。
复杂业务放在中断里，代码难维护，也更容易引入时序问题。
```

所以在 RTOS 里，更常见的设计是：

```text
中断负责快速捕获事件。
任务负责真正处理业务。
```

也可以理解成：

```text
中断负责“叫醒任务”。
任务负责“处理事情”。
```

本课采用的就是这种结构：

```text
按键产生 EXTI 中断 -> 中断里清除 pending 标志 -> 中断里用 xTaskNotifyFromISR() 通知 KeyWaitTask -> KeyWaitTask 被唤醒 -> 在任务上下文里翻转 LED、打印串口、处理业务
```

## 为什么不只用单独中断

如果只用中断，代码可能是：

```text
KEY_IRQHandler() -> 判断按键 -> 翻转 LED -> 打印串口 -> 做业务处理
```

这种写法在裸机小实验里可以接受，但在 RTOS 项目里会逐渐暴露问题：

```text
中断处理时间变长，其他中断和任务响应变差。
串口打印等慢操作放在中断里，会拖住 CPU。
业务逻辑堆在 IRQHandler 里，后期难扩展。
如果需要等待资源，中断里不能像任务一样阻塞。
```

所以单独中断适合：

```text
极短、极简单、确定不会阻塞的处理。
```

例如：

```text
清标志位。
记录一个时间戳。
把一个字节放进环形缓冲区。
设置一个标志位。
```

## 为什么不只用单独任务

如果只用任务，不用中断，代码可能是：

```text
KeyTask -> 每隔 10ms 读取按键 GPIO -> 判断是否按下
```

这种方式也能工作，但它是轮询模型。

问题是：

```text
任务检查得太慢，响应就慢。
任务检查得太快，CPU 浪费就多。
对串口、DMA、ADC 这类外设事件，纯轮询通常不够优雅。
```

单独任务适合：

```text
周期性检查。
对响应速度要求不高。
事件频率低，且允许一定延迟。
```

但如果外设事件需要及时响应，就应该让中断先捕获事件。

## 中断 + 任务的好处

把中断和任务结合起来，可以同时得到两边的优点：

```text
中断：负责快速响应硬件事件。
任务：负责可阻塞、可调度、可维护的业务处理。
```

具体好处：

```text
响应及时：事件一来，中断马上捕获。
中断短小：IRQHandler 只清标志、发通知，很快退出。
任务可调度：后续处理可以交给高优先级任务尽快运行。
代码清晰：硬件事件入口和业务处理逻辑分开。
更安全：任务里可以使用普通 FreeRTOS API，中断里只用 FromISR API。
更适合扩展：以后按键消抖、长按短按、状态机都可以放在任务里。
```

这个模型在大型系统里非常常见，可以类比 Linux：

```text
硬中断 top half：尽快确认事件，做最少工作。
下半部 bottom half / workqueue：处理较重逻辑。
```

在 FreeRTOS 里，对应关系可以理解为：

```text
ISR：top half。
Task：bottom half。
FromISR API：ISR 和 Task 之间的安全桥梁。
```

## 这个模式适合什么场景

适合这种场景：

```text
事件来自硬件外设。
事件需要及时响应。
事件后续处理不适合放在中断里。
后续处理可能涉及日志、协议解析、状态机、共享资源或较长计算。
```

典型例子：

```text
按键中断：中断通知任务，任务里做消抖、短按、长按、菜单逻辑。
串口接收中断：中断接收字节或通知任务，任务里解析协议。
DMA 完成中断：中断通知任务，任务里处理整块数据。
ADC 完成中断：中断通知任务，任务里滤波、计算、上传。
传感器 INT 中断：中断通知任务，任务里通过 I2C/SPI 读取数据。
定时器捕获中断：中断记录时间或通知任务，任务里计算频率、周期、脉宽。
```

本课用按键只是为了让现象容易观察。

真正要学习的是这个 RTOS 外设处理模型：

```text
外设事件 -> ISR 快速响应 -> FromISR API 通知任务 -> 任务完成业务处理
```

## 本课目标

上一课 Lesson 21 学的是任务和任务之间的通知：

```text
NotifyTask -> xTaskNotifyGive() -> WaitTask -> ulTaskNotifyTake()
```

这一课把通知源从“任务”换成“中断”：

```text
按键产生 EXTI 中断 -> 中断里通知任务 -> 任务被唤醒后处理按键事件
```

本课目标是观察：

```text
按 K1 -> EXTI0_IRQHandler() -> xTaskNotifyFromISR() -> KeyWaitTask 被唤醒 -> 打印 bits=0x1 并翻转红灯
按 K2 -> EXTI15_10_IRQHandler() -> xTaskNotifyFromISR() -> KeyWaitTask 被唤醒 -> 打印 bits=0x2 并翻转绿灯
不按键 -> HeartbeatTask 每 2000ms 打印 system running 并翻转蓝灯
```

这节课的核心不是按键本身，而是：

```text
中断如何安全地唤醒 FreeRTOS 任务。
```

## 对比上一个 lesson

对比 Lesson 21，本课不再由 `NotifyTask` 周期性发送通知。

本课变化：

```text
User/main.c 新增 EXTI_Key_Config()。
User/main.c 新增 KeyWaitTask()。
User/main.c 新增 HeartbeatTask()。
User/main.c 新增 NotifyKeyTaskFromISR()。
User/main.c 使用 xTaskNotifyWait() 等待按键 bit。
User/stm32f10x_it.c 新增 KEY1_IRQHandler() 和 KEY2_IRQHandler()。
User/Key 新增 bsp_exti.c 和 bsp_exti.h。
Makefile 新增 User/Key/bsp_exti.c。
Makefile 新增 Libraries/FWlib/src/stm32f10x_exti.c。
```

这部分实现的功能：

```text
KEY1 中断：设置 KEY1_NOTIFY_BIT。
KEY2 中断：设置 KEY2_NOTIFY_BIT。
KeyWaitTask：等待按键通知 bit，并在任务上下文中处理 LED 和串口打印。
HeartbeatTask：周期性输出心跳，证明系统没有被中断处理卡住。
```

## 为什么中断里要用 FromISR API

普通任务上下文中可以调用：

```c
xTaskNotifyGive();
xTaskNotify();
xTaskNotifyWait();
ulTaskNotifyTake();
```

但中断服务函数里不能随便调用普通任务 API。

原因是：

```text
中断不是普通任务。
中断没有任务栈、任务状态、任务优先级和阻塞态。
中断不能像任务一样等待资源。
中断里执行时间应该尽量短。
```

所以 FreeRTOS 给很多 API 准备了 ISR 版本：

```text
xTaskNotifyFromISR()
xTaskNotifyGiveFromISR()
xQueueSendFromISR()
xSemaphoreGiveFromISR()
```

命名规则很直接：

```text
FromISR = 这个函数允许在中断服务函数里调用。
```

一句话：

```text
任务里用普通 API，中断里用 FromISR API。
```

## 本课为什么用 xTaskNotifyFromISR()

上一课用的是：

```c
xTaskNotifyGive(wait_task_handle);
ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
```

这适合表达：

```text
只通知一次，不区分具体事件来源。
```

本课有两个按键：

```text
K1
K2
```

如果只用 `xTaskNotifyGiveFromISR()`，任务只知道“有通知来了”，但不知道是 K1 还是 K2。

所以本课使用更通用的：

```c
xTaskNotifyFromISR()
```

并配合：

```c
eSetBits
```

把通知值当成 bit 位使用：

```text
KEY1_NOTIFY_BIT = 0x01
KEY2_NOTIFY_BIT = 0x02
```

这样任务收到通知后可以判断：

```text
bits=0x1 -> K1
bits=0x2 -> K2
bits=0x3 -> K1 和 K2 都发生过
```

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印 lesson-22-from-isr-notification start -> EXTI_Key_Config() -> CreateTasks() -> vTaskStartScheduler()
```

其中：

```text
EXTI_Key_Config()：配置 K1/K2 的 GPIO、EXTI 线和 NVIC 中断优先级。
CreateTasks()：创建 KeyWaitTask 和 HeartbeatTask。
```

## 通知 bit 定义

代码：

```c
#define KEY1_NOTIFY_BIT ( ( uint32_t ) 0x01 )
#define KEY2_NOTIFY_BIT ( ( uint32_t ) 0x02 )
#define ALL_KEY_NOTIFY_BITS ( KEY1_NOTIFY_BIT | KEY2_NOTIFY_BIT )
```

二进制理解：

```text
KEY1_NOTIFY_BIT = 0x01 = 0000 0001
KEY2_NOTIFY_BIT = 0x02 = 0000 0010
```

组合起来：

```text
ALL_KEY_NOTIFY_BITS = 0x03 = 0000 0011
```

这和 Lesson 19 的事件组很像。

区别是：

```text
事件组：bit 存在 EventGroup 对象里。
任务通知：bit 存在目标任务自己的 TCB 通知值里。
```

## NotifyKeyTaskFromISR()

完整函数：

```c
void NotifyKeyTaskFromISR(uint32_t key_bits)
{
  BaseType_t higher_priority_task_woken = pdFALSE;

  if (key_wait_task_handle != NULL)
  {
    (void)xTaskNotifyFromISR(key_wait_task_handle,
                             key_bits,
                             eSetBits,
                             &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
  }
}
```

横向流程：

```text
中断传入 key_bits -> 准备 higher_priority_task_woken -> 确认 key_wait_task_handle 有效 -> xTaskNotifyFromISR() 设置任务通知 bit -> 如果唤醒了更高优先级任务，portYIELD_FROM_ISR() 请求中断退出后立刻切换任务
```

带详细注释的版本：

```c
void NotifyKeyTaskFromISR(uint32_t key_bits)
{
  /* 先假设这次中断没有唤醒更高优先级任务。 */
  BaseType_t higher_priority_task_woken = pdFALSE;

  /*
   * key_wait_task_handle 是 KeyWaitTask 的任务句柄。
   * 只有目标任务已经创建成功，句柄有效，才可以通知它。
   */
  if (key_wait_task_handle != NULL)
  {
    /*
     * 从中断上下文通知 KeyWaitTask。
     *
     * key_wait_task_handle：
     *   目标任务，也就是要被唤醒的 KeyWaitTask。
     *
     * key_bits：
     *   本次按键中断带来的事件 bit。
     *   K1 是 0x01，K2 是 0x02。
     *
     * eSetBits：
     *   把 key_bits 按位 OR 到 KeyWaitTask 的通知值里。
     *
     * &higher_priority_task_woken：
     *   如果这次通知唤醒了更高优先级任务，FreeRTOS 会把它改成 pdTRUE。
     */
    (void)xTaskNotifyFromISR(key_wait_task_handle,
                             key_bits,
                             eSetBits,
                             &higher_priority_task_woken);

    /*
     * 如果刚才唤醒了更高优先级任务，就请求中断退出后立刻切换任务。
     * 在 Cortex-M3 上，这通常会触发 PendSV 来完成上下文切换。
     */
    portYIELD_FROM_ISR(higher_priority_task_woken);
  }
}
```

逐行理解：

```text
void NotifyKeyTaskFromISR(uint32_t key_bits)
```

定义一个给中断调用的通知函数。`key_bits` 表示这次中断要传递给任务的按键事件：

```text
K1 -> KEY1_NOTIFY_BIT -> 0x01
K2 -> KEY2_NOTIFY_BIT -> 0x02
```

```text
BaseType_t higher_priority_task_woken = pdFALSE;
```

创建一个变量，用来接收 FreeRTOS 的判断结果。初始值是 `pdFALSE`，表示先假设没有唤醒更高优先级任务。

```text
if (key_wait_task_handle != NULL)
```

确认目标任务句柄有效。因为任务通知必须知道目标任务是谁，如果句柄是 `NULL`，就不能通知。

```text
xTaskNotifyFromISR(...)
```

从中断里通知目标任务。这里不能使用普通的 `xTaskNotify()`，因为当前调用者是中断服务函数。

```text
eSetBits
```

表示把本次事件 bit 合并到任务通知值里。它不是覆盖，而是按位 OR：

```text
当前通知值 | key_bits
```

所以如果 K1 和 K2 都发生过，任务可能收到：

```text
0x01 | 0x02 = 0x03
```

```text
&higher_priority_task_woken
```

把变量地址交给 FreeRTOS。若这次中断唤醒了一个比当前任务优先级更高的任务，FreeRTOS 会把它改成 `pdTRUE`。

```text
portYIELD_FROM_ISR(higher_priority_task_woken)
```

如果 `higher_priority_task_woken` 是 `pdTRUE`，就请求中断退出后进行任务切换，让刚刚被唤醒的高优先级任务尽快运行。

完整例子：

```text
当前 HeartbeatTask 正在运行，优先级 1
KeyWaitTask 正在 xTaskNotifyWait() 中阻塞，优先级 3
K1 按下，进入 EXTI0 中断
NotifyKeyTaskFromISR(0x01)
xTaskNotifyFromISR() 把 0x01 设置到 KeyWaitTask 的通知值里
KeyWaitTask 从阻塞态变为就绪态
因为 KeyWaitTask 优先级更高，higher_priority_task_woken 变成 pdTRUE
portYIELD_FROM_ISR(pdTRUE) 请求切换
中断退出后切换到 KeyWaitTask
KeyWaitTask 读取 key_bits=0x1，处理 K1 事件
```

## xTaskNotifyFromISR()

参数指南：

```c
(void)xTaskNotifyFromISR(
    key_wait_task_handle,          /* 要通知哪个任务 */
    key_bits,                      /* 要写入通知值的数据，本课是 KEY1/KEY2 bit */
    eSetBits,                      /* 写入方式：把 key_bits OR 到任务通知值里 */
    &higher_priority_task_woken    /* 是否唤醒了更高优先级任务 */
);
```

第 1 个参数：

```text
key_wait_task_handle
```

表示目标任务。

任务通知不是广播，它必须知道：

```text
我要通知哪一个任务。
```

第 2 个参数：

```text
key_bits
```

表示这次中断要传递的信息。

本课：

```text
K1 -> 0x01
K2 -> 0x02
```

第 3 个参数：

```text
eSetBits
```

表示把 `key_bits` 按 bit OR 到目标任务的通知值中。

例如当前通知值是：

```text
0000 0000
```

K1 中断来了：

```text
0000 0000 | 0000 0001 = 0000 0001
```

K2 又来了：

```text
0000 0001 | 0000 0010 = 0000 0011
```

第 4 个参数：

```text
&higher_priority_task_woken
```

它用来告诉我们：

```text
这次中断通知是否唤醒了一个更高优先级任务。
```

如果是，就应该在中断退出前请求一次任务切换。

## higher_priority_task_woken

代码：

```c
BaseType_t higher_priority_task_woken = pdFALSE;
```

它的含义是：

```text
中断调用 FromISR API 之前，先假设没有唤醒更高优先级任务。
```

然后传给：

```c
xTaskNotifyFromISR(..., &higher_priority_task_woken);
```

如果 `KeyWaitTask` 原来正在阻塞等待通知，并且它优先级高于当前正在运行的任务，那么 FreeRTOS 会把这个变量改成：

```text
pdTRUE
```

这表示：

```text
中断刚刚唤醒了一个更高优先级任务。
```

## portYIELD_FROM_ISR()

代码：

```c
portYIELD_FROM_ISR(higher_priority_task_woken);
```

横向理解：

```text
如果 higher_priority_task_woken == pdTRUE -> 请求 PendSV -> 中断退出后立刻进行任务切换
```

为什么需要它？

因为中断发生时，CPU 原本可能正在运行低优先级任务，例如 `HeartbeatTask`。

按键中断来了：

```text
EXTI IRQ -> xTaskNotifyFromISR() -> KeyWaitTask 被唤醒
```

此时 `KeyWaitTask` 优先级是 3，`HeartbeatTask` 优先级是 1。

如果不请求切换，CPU 可能要等到下一个调度点才切到 `KeyWaitTask`。

如果调用：

```c
portYIELD_FROM_ISR(pdTRUE);
```

则链路变成：

```text
中断唤醒高优先级任务 -> 设置 PendSV -> 中断退出 -> 立刻进入 PendSV -> 切换到 KeyWaitTask
```

一句话：

```text
portYIELD_FROM_ISR() 让被中断唤醒的高优先级任务尽快运行。
```

## KEY1_IRQHandler()

完整函数：

```c
void KEY1_IRQHandler(void)
{
  if (EXTI_GetITStatus(KEY1_INT_EXTI_LINE) != RESET)
  {
    EXTI_ClearITPendingBit(KEY1_INT_EXTI_LINE);
    NotifyKeyTaskFromISR(KEY1_NOTIFY_BIT);
  }
}
```

横向流程：

```text
K1 触发 EXTI0 中断 -> 进入 KEY1_IRQHandler() -> 确认 EXTI Line0 真的 pending -> 清除 EXTI pending 标志 -> 通知 KeyWaitTask：KEY1_NOTIFY_BIT
```

注意：

```text
中断里没有串口打印。
中断里没有处理 LED。
中断里只清标志并通知任务。
```

这是 RTOS 下更推荐的写法：

```text
中断里做少量、快速、必要的工作。
复杂处理放到任务里做。
```

## KEY2_IRQHandler()

完整函数：

```c
void KEY2_IRQHandler(void)
{
  if (EXTI_GetITStatus(KEY2_INT_EXTI_LINE) != RESET)
  {
    EXTI_ClearITPendingBit(KEY2_INT_EXTI_LINE);
    NotifyKeyTaskFromISR(KEY2_NOTIFY_BIT);
  }
}
```

横向流程：

```text
K2 触发 EXTI15_10 中断 -> 进入 KEY2_IRQHandler() -> 确认 EXTI Line13 pending -> 清除 EXTI pending 标志 -> 通知 KeyWaitTask：KEY2_NOTIFY_BIT
```

## KeyWaitTask

完整函数：

```c
static void KeyWaitTask(void *argument)
{
  uint32_t key_bits;

  (void)argument;

  while (1)
  {
    key_bits = 0;
    (void)xTaskNotifyWait(0,
                          ALL_KEY_NOTIFY_BITS,
                          &key_bits,
                          portMAX_DELAY);

    if ((key_bits & KEY1_NOTIFY_BIT) != 0)
    {
      LED1_TOGGLE;
    }

    if ((key_bits & KEY2_NOTIFY_BIT) != 0)
    {
      LED2_TOGGLE;
    }

    SendKeyLog(key_bits);
  }
}
```

横向流程：

```text
KeyWaitTask 运行 -> xTaskNotifyWait() 阻塞等待通知 bit -> 中断通知到来 -> xTaskNotifyWait() 返回 key_bits -> 判断是否包含 KEY1 bit -> 判断是否包含 KEY2 bit -> 翻转对应 LED -> 打印 key_bits -> 进入下一轮等待
```

## xTaskNotifyWait()

参数指南：

```c
(void)xTaskNotifyWait(
    0,                    /* 进入等待前要清除哪些 bit；0 表示进入前不清 */
    ALL_KEY_NOTIFY_BITS,  /* 退出等待时要清除哪些 bit；这里清除 K1/K2 bit */
    &key_bits,            /* 返回收到的通知值 */
    portMAX_DELAY         /* 没有通知就一直阻塞等待 */
);
```

第 1 个参数：

```text
0
```

进入等待前不主动清 bit。

第 2 个参数：

```text
ALL_KEY_NOTIFY_BITS
```

任务成功收到通知并退出等待时，清除 K1/K2 对应 bit。

第 3 个参数：

```text
&key_bits
```

把这次收到的通知值保存出来。

第 4 个参数：

```text
portMAX_DELAY
```

没有按键中断就一直阻塞，不占用 CPU。

## HeartbeatTask

本课还保留一个心跳任务：

```c
static void HeartbeatTask(void *argument)
{
  (void)argument;

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(2000));
    LED3_TOGGLE;
    SendTaskLog("HeartbeatTask", "system running");
  }
}
```

它的作用是：

```text
不按键时，系统仍然每 2000ms 输出一次心跳。
按键中断发生后，KeyWaitTask 能及时抢占低优先级心跳任务。
```

## 中断优先级配置

FreeRTOS 对“能调用 FromISR API 的中断”有优先级要求。

本课配置：

```c
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
```

对应 `FreeRTOSConfig.h`：

```c
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
```

含义：

```text
数值越小，中断硬件优先级越高。
数值 0 是最高优先级。
数值 15 是最低优先级。
```

FreeRTOS 规则是：

```text
调用 FromISR API 的中断，优先级数值必须 >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY。
```

本课按键中断优先级是：

```text
5
```

所以它可以调用：

```text
xTaskNotifyFromISR()
```

如果把按键中断优先级设成 1，就不应该调用 FreeRTOS 的 FromISR API，否则可能触发断言或产生难以定位的问题。

## 完整运行链路

系统启动：

```text
main() -> 初始化 LED/USART/EXTI -> 创建 KeyWaitTask 和 HeartbeatTask -> 启动调度器
```

不按键：

```text
KeyWaitTask -> xTaskNotifyWait() -> 没有通知 -> 阻塞
HeartbeatTask -> 每 2000ms 打印 system running
```

按 K1：

```text
K1 产生上升沿 -> EXTI0 pending -> CPU 进入 KEY1_IRQHandler() -> 清除 EXTI pending -> xTaskNotifyFromISR(key_wait_task_handle, KEY1_NOTIFY_BIT, eSetBits, ...) -> KeyWaitTask 被唤醒 -> portYIELD_FROM_ISR() 请求切换 -> 中断退出 -> KeyWaitTask 运行 -> key_bits=0x1 -> LED1_TOGGLE -> 打印 notify bits=0x1
```

按 K2：

```text
K2 产生下降沿 -> EXTI13 pending -> CPU 进入 KEY2_IRQHandler() -> 清除 EXTI pending -> xTaskNotifyFromISR(key_wait_task_handle, KEY2_NOTIFY_BIT, eSetBits, ...) -> KeyWaitTask 被唤醒 -> 中断退出后切换到 KeyWaitTask -> key_bits=0x2 -> LED2_TOGGLE -> 打印 notify bits=0x2
```

快速按下 K1 和 K2：

```text
K1 设置 bit 0 -> K2 设置 bit 1 -> KeyWaitTask 读取通知值 -> 可能看到 key_bits=0x3
```

这说明任务通知也可以像事件组一样，用 bit 表达多个事件。

## 预期串口现象

Reset 后先输出：

```text
lesson-22-from-isr-notification start
```

不按键时：

```text
[tick 2000] HeartbeatTask: system running
[tick 4000] HeartbeatTask: system running
```

按 K1：

```text
[tick 5234] KeyWaitTask: notify bits=0x1
```

按 K2：

```text
[tick 7120] KeyWaitTask: notify bits=0x2
```

如果很快连续按 K1/K2，可能看到：

```text
[tick 9000] KeyWaitTask: notify bits=0x3
```

## 本课结论

这一课要建立六个判断：

```text
1. 中断里要调用 FreeRTOS 的 FromISR API，而不是普通任务 API。
2. xTaskNotifyFromISR() 可以在中断中通知任务。
3. xTaskNotifyWait() 可以在任务中等待中断发来的通知。
4. pxHigherPriorityTaskWoken 用来判断是否唤醒了更高优先级任务。
5. portYIELD_FROM_ISR() 用来让被唤醒的高优先级任务尽快运行。
6. 调用 FromISR API 的中断优先级必须符合 configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 规则。
```

一句话：

```text
RTOS 下的中断应该尽快把事件交给任务处理，而不是在中断里做复杂业务。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd '/home/yang/stm32-FreeRTOS/lessons/lesson-22-from-isr-notification/26-FreeRTOS中断任务通知'
make clean
make
```

生成产物：

```text
build/lesson-22-from-isr-notification.elf
build/lesson-22-from-isr-notification.hex
build/lesson-22-from-isr-notification.bin
build/lesson-22-from-isr-notification.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
