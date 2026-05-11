# Lesson 08: 空闲任务与阻塞延时

## 本课定位

前面已经建立了几件事：

```text
lesson-04：SysTick 可以产生系统节拍
lesson-06：任务可以通过保存/恢复上下文来切换
lesson-07：共享资源需要临界段保护
```

这一课解决一个非常重要的问题：

```text
任务想“等一段时间”时，CPU 应该怎么办？
```

裸机里我们已经写过 `Delay_ms()`，但 RTOS 里的延时不是简单空等，而是让任务进入阻塞态。

## 对应资料

```text
FreeRTOS 内核实现与应用开发实战指南
第 9 章：空闲任务与阻塞延时的实现
```

对应配套例程：

```text
Cortex-M3/09，空闲任务与阻塞延时的实现
```

## 先回顾裸机 Delay_ms()

lesson-04 中的逻辑是：

```text
main() -> LED3_TOGGLE -> 打印 tick -> Delay_ms(500) -> 重复
```

`Delay_ms(500)` 的本质是：

```text
记录开始 tick
-> 一直循环检查当前 tick 是否已经增加 500
-> 到时间后返回
```

横向流程：

```text
进入 Delay_ms(500)
-> main() 卡在 while 判断里
-> SysTick 每 1ms 中断一次，让 tick 增加
-> tick 差值达到 500
-> Delay_ms() 返回
```

它的优点：

```text
比空循环精确
容易理解
裸机阶段够用
```

它的问题：

```text
当前执行流被卡住
CPU 大部分时间在反复检查条件
如果有多个功能都要等待，程序结构会变差
```

所以：

```text
Delay_ms() 是阻塞当前执行流的等待
```

## RTOS 为什么不能继续空等

RTOS 的目标之一是提高 CPU 利用率。

如果一个任务要等 500ms，它不应该霸占 CPU 做空循环。

更合理的做法是：

```text
这个任务告诉调度器：我 500ms 后再继续运行
然后主动让出 CPU
```

这就是阻塞延时。

## 什么是阻塞延时

阻塞延时的核心是：

```text
任务等待期间，不占用 CPU
```

以 `vTaskDelay(500)` 为例，直觉流程是：

```text
当前任务调用 vTaskDelay(500)
-> 当前任务进入阻塞态
-> 调度器不再选择它运行
-> CPU 去运行其他就绪任务
-> 500 个 tick 到期
-> 当前任务重新进入就绪态
-> 调度器合适时再次运行它
```

这和裸机 `Delay_ms(500)` 的区别非常大。

对比：

```text
Delay_ms(500)：当前代码一直占着执行流等待
vTaskDelay(500)：当前任务睡眠，CPU 可以运行别的任务
```

## 任务状态的最小理解

现阶段先掌握三个状态：

```text
运行态：正在占用 CPU 执行
就绪态：可以运行，但 CPU 当前没轮到它
阻塞态：暂时不能运行，正在等待时间或事件
```

以 LED 任务为例：

```c
void LedTask(void *argument)
{
    while (1)
    {
        LED3_TOGGLE;
        vTaskDelay(500);
    }
}
```

横向流程：

```text
LED 任务运行态
-> 执行 LED3_TOGGLE
-> 调用 vTaskDelay(500)
-> LED 任务进入阻塞态
-> 500ms 到期
-> LED 任务进入就绪态
-> 调度器再次选择它
-> LED 任务回到运行态
```

## vTaskDelay() 最小模型

教材这一章的简化模型里，`vTaskDelay()` 大概做两件事：

```text
记录当前任务要延时多少 tick
主动触发一次任务切换
```

伪代码：

```c
void vTaskDelay(TickType_t xTicksToDelay)
{
    TCB_t *pxTCB;

    pxTCB = pxCurrentTCB;
    pxTCB->xTicksToDelay = xTicksToDelay;

    taskYIELD();
}
```

横向理解：

```text
获取当前任务 TCB
-> 在 TCB 中记录 xTicksToDelay
-> 主动让出 CPU
-> 触发 PendSV
-> 进入任务切换
```

注意这里的关键：

```text
延时信息记录在 TCB 里
```

也就是说，任务不是简单“睡着了”，而是调度器知道：

```text
这个任务还要等多少 tick 才能重新运行
```

## SysTick 在阻塞延时中的作用

SysTick 仍然是系统时间来源。

每来一次 SysTick：

```text
系统 tick 增加
-> 检查阻塞任务的延时时间
-> 把还没到期的任务继续留在阻塞态
-> 把到期的任务移回就绪态
```

教材简化模型中，可以理解为：

```text
每个 SysTick 周期，让阻塞任务的 xTicksToDelay 减 1
```

当：

```text
xTicksToDelay == 0
```

任务就可以重新参与调度。

完整横向链路：

```text
Task1 调用 vTaskDelay(500)
-> Task1TCB.xTicksToDelay = 500
-> Task1 让出 CPU
-> SysTick 每 1ms 到来
-> Task1TCB.xTicksToDelay 逐渐减少
-> 减到 0
-> Task1 从阻塞态回到就绪态
-> 调度器再次选择 Task1
```

## 什么是空闲任务

如果所有普通任务都在阻塞态，CPU 不能没有代码可执行。

所以 RTOS 会创建一个最低优先级的任务：

```text
空闲任务
```

空闲任务的作用：

```text
保证系统永远有一个任务可以运行
在真实 FreeRTOS 中处理一些系统清理工作
在低功耗系统中可以进入休眠
```

最小形式：

```c
void prvIdleTask(void *argument)
{
    while (1)
    {
        idle_count++;
    }
}
```

它不是“无意义”的任务，而是 RTOS 的兜底任务。

## 为什么一定要有空闲任务

调度器的工作是选择一个任务运行。

如果没有空闲任务，可能出现：

```text
Task1 在延时
Task2 在延时
Task3 在等待事件
```

此时没有普通任务处于就绪态。

但 CPU 不能停止在空白状态，所以需要：

```text
空闲任务保持就绪
```

这样调度器永远可以找到一个任务。

横向流程：

```text
所有普通任务都阻塞
-> 调度器找不到普通就绪任务
-> 选择空闲任务
-> CPU 运行空闲任务
-> SysTick 继续推进时间
-> 某个任务延时到期
-> 该任务回到就绪态
-> 调度器切回普通任务
```

## 空闲任务也有栈和 TCB

空闲任务本质上也是一个任务。

所以它也需要：

```text
任务栈
任务控制块 TCB
任务入口函数
```

教材中的简化模型：

```c
#define configMINIMAL_STACK_SIZE 128

StackType_t IdleTaskStack[configMINIMAL_STACK_SIZE];
TCB_t IdleTaskTCB;
```

创建空闲任务时，需要把这些内存交给内核：

```c
void vApplicationGetIdleTaskMemory(
    TCB_t **ppxIdleTaskTCBBuffer,
    StackType_t **ppxIdleTaskStackBuffer,
    uint32_t *pulIdleTaskStackSize )
{
    *ppxIdleTaskTCBBuffer = &IdleTaskTCB;
    *ppxIdleTaskStackBuffer = IdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
```

横向理解：

```text
用户提供 IdleTaskTCB 和 IdleTaskStack
-> 内核拿到这两块内存
-> 创建空闲任务
-> 把空闲任务加入最低优先级就绪列表
```

## 一个具体运行场景

假设系统有两个任务：

```text
LED 任务：每 500ms 翻转一次 LED
USART 任务：每 1000ms 打印一次日志
```

启动后：

```text
LED 任务运行
-> LED 翻转
-> vTaskDelay(500)
-> LED 任务阻塞
-> 调度器切到 USART 任务
-> USART 打印
-> vTaskDelay(1000)
-> USART 任务阻塞
-> 现在普通任务都阻塞
-> 调度器运行空闲任务
-> SysTick 每 1ms 继续推进时间
-> 500ms 到，LED 任务解除阻塞
-> 调度器切回 LED 任务
```

从外部看：

```text
LED 按 500ms 周期闪烁
串口按 1000ms 周期打印
CPU 空余时运行空闲任务
```

这就是 RTOS 比裸机 `Delay_ms()` 更强的地方。

## Delay_ms() 和 vTaskDelay() 的本质区别

最终要记住这张对比：

```text
Delay_ms()
当前执行流一直等
等待期间主要反复检查 tick
不主动让出 CPU 给其他业务
适合裸机简单程序
```

```text
vTaskDelay()
当前任务进入阻塞态
等待期间不占用 CPU
调度器可以运行其他任务或空闲任务
适合 RTOS 多任务程序
```

一句话：

```text
Delay_ms() 是“我在这里等”
vTaskDelay() 是“我先睡，到点再叫我”
```

## 和前面课程的关系

```text
lesson-04：SysTick 提供 1ms 时间基础
lesson-06：任务可以通过上下文切换让出 CPU
lesson-07：切换和共享数据需要临界段保护
lesson-08：任务可以主动阻塞，让 CPU 运行其他任务或空闲任务
```

## 本课总结

这节课记住五句话：

```text
裸机 Delay_ms() 会阻塞当前执行流
RTOS vTaskDelay() 会让当前任务进入阻塞态
任务阻塞期间不占用 CPU
如果没有其他就绪任务，CPU 运行空闲任务
SysTick 推进系统时间，并让延时到期的任务重新就绪
```

下一课建议进入：

```text
Lesson 09：建立 STM32 FreeRTOS 工程模板
```

下一课开始从概念回到板子代码，把 FreeRTOS 真正放进 STM32F103 工程里。
