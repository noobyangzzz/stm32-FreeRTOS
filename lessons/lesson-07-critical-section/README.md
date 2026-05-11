# Lesson 07: 临界段保护

## 本课定位

前面 lesson-06 已经建立了任务切换的最小模型：

```text
保存当前任务上下文 -> 切换当前 TCB -> 恢复下一个任务上下文
```

一旦系统里存在多个任务，或者任务和中断都会访问同一份数据，就会出现新的问题：

```text
一段代码执行到一半时被打断，另一个任务或中断也访问了同一份数据。
```

这节课要理解的核心就是：

```text
哪些代码不能被打断？
如何保护这些代码？
为什么 FreeRTOS 最终要通过屏蔽中断来保护临界段？
```

## 对应资料

```text
FreeRTOS 内核实现与应用开发实战指南
第 8 章：临界段的保护
```

对应配套例程：

```text
Cortex-M3/08，临界段的保护
```

## 什么是临界资源

临界资源就是可能被多个执行流共同访问的资源。

这里的执行流包括：

```text
任务
中断服务函数
```

常见临界资源：

```text
全局变量
共享缓冲区
链表
队列内部数据结构
外设寄存器
某个状态标志位
```

只要一个资源可能被多个地方同时读写，就要考虑它是不是临界资源。

## 什么是临界段

临界段就是访问临界资源时，必须完整执行、不能被打断的一小段代码。

例子：

```c
shared_count++;
```

这一句看起来像一个动作，但 CPU 执行时通常不是一步完成，而是类似：

```text
读取 shared_count
-> 加 1
-> 写回 shared_count
```

这叫“读-改-写”过程。

如果这个过程执行到一半被打断，就可能出错。

## 一个具体错误场景

假设当前：

```text
shared_count = 0
```

Task1 和 Task2 都要执行：

```c
shared_count++;
```

没有保护时可能发生：

```text
Task1 读取 shared_count = 0
-> Task1 计算 0 + 1，准备写回 1
-> Task1 被切走
-> Task2 读取 shared_count = 0
-> Task2 计算 0 + 1
-> Task2 写回 shared_count = 1
-> Task1 恢复
-> Task1 按刚才算好的结果写回 shared_count = 1
```

最终结果：

```text
两个任务都执行了一次 shared_count++
但 shared_count 最后只等于 1
```

正确结果本应是：

```text
shared_count = 2
```

这就是临界段问题。

## 为什么会出错

根本原因是：

```text
shared_count++ 不是不可分割操作
```

它包含：

```text
读
改
写
```

如果系统在“读”和“写”之间发生任务切换或中断，就可能破坏数据一致性。

所以需要保护的不是整个任务，而是这段访问共享数据的短代码。

## 加上临界段保护后

保护后的横向流程：

```text
进入临界段
-> 读取 shared_count
-> 加 1
-> 写回 shared_count
-> 退出临界段
```

此时如果 Task1 正在临界段里执行：

```text
Task1 进入临界段
-> Task1 读取 shared_count = 0
-> Task1 写回 shared_count = 1
-> Task1 退出临界段
-> Task2 才能进入临界段
-> Task2 读取 shared_count = 1
-> Task2 写回 shared_count = 2
```

最终结果：

```text
shared_count = 2
```

## 临界段保护在 FreeRTOS 中怎么做

FreeRTOS 里常见形式是：

```c
taskENTER_CRITICAL();

/* 访问共享资源 */
shared_count++;

taskEXIT_CRITICAL();
```

横向理解：

```text
taskENTER_CRITICAL()
-> 屏蔽会影响 FreeRTOS 的中断
-> 临界段代码完整执行
-> taskEXIT_CRITICAL()
-> 恢复中断屏蔽状态
```

这里的重点不是 API 名字，而是思想：

```text
进入临界段时，暂时禁止会破坏当前操作的一类打断
退出临界段后，恢复系统正常响应
```

## 为什么最终要回到中断屏蔽

前面 lesson-06 说过，任务切换通常发生在异常/中断机制里：

```text
SysTick -> 判断是否需要调度 -> PendSV -> 上下文切换
```

也就是说，任务切换最终也和异常/中断有关。

所以如果一段代码确实不能被任务切换打断，FreeRTOS 需要在底层阻止相关中断触发切换。

但是 FreeRTOS 并不是简单粗暴地关闭所有中断。

在 Cortex-M 中，FreeRTOS 主要通过 `BASEPRI` 来屏蔽一部分中断。

## PRIMASK、FAULTMASK、BASEPRI 的直觉区别

Cortex-M 有几类中断屏蔽机制。

现阶段先建立直觉：

```text
PRIMASK：一刀切，屏蔽大多数可屏蔽中断
FAULTMASK：更强，连很多 fault 都屏蔽，只剩 NMI
BASEPRI：设置优先级阈值，只屏蔽一部分低优先级中断
```

FreeRTOS 更偏向使用 `BASEPRI`，因为它可以做到：

```text
屏蔽会调用 FreeRTOS API、会影响内核数据结构的中断
保留真正高优先级、紧急的中断响应能力
```

这就是为什么它比“全关中断”更细。

## BASEPRI 的横向理解

可以把 `BASEPRI` 理解成一个门槛：

```text
优先级号 >= BASEPRI 门槛的中断被屏蔽
优先级号 < BASEPRI 门槛的中断仍然可以响应
```

注意 Cortex-M 的优先级号规则：

```text
数字越小，优先级越高
数字越大，优先级越低
```

所以设置 `BASEPRI` 后：

```text
高于门槛的紧急中断可以继续响应
低于或等于门槛的普通中断暂时被屏蔽
```

## 不带返回值和带返回值的关中断

资料里提到两类关中断：

```text
不带返回值：portDISABLE_INTERRUPTS()
带返回值：portSET_INTERRUPT_MASK_FROM_ISR()
```

直觉区别：

```text
不带返回值：直接设置新的屏蔽级别，不关心原来是什么状态
带返回值：先保存原来的屏蔽状态，再设置新的屏蔽级别
```

为什么带返回值更适合中断里使用？

因为中断发生前系统可能已经处在某种屏蔽状态。

带返回值版本的横向流程：

```text
读取当前 BASEPRI
-> 保存旧 BASEPRI
-> 设置新 BASEPRI，进入保护状态
-> 执行临界段
-> 用旧 BASEPRI 恢复原状态
```

也就是：

```text
进入前是什么中断屏蔽状态，退出时就恢复成什么状态
```

## 临界段嵌套

临界段可能嵌套：

```c
taskENTER_CRITICAL();

/* 外层临界段 */

taskENTER_CRITICAL();
/* 内层临界段 */
taskEXIT_CRITICAL();

taskEXIT_CRITICAL();
```

如果内层 `taskEXIT_CRITICAL()` 直接开中断，就会破坏外层临界段。

所以 FreeRTOS 会维护一个嵌套计数，例如：

```text
第一次进入临界段 -> nesting = 1，关中断
第二次进入临界段 -> nesting = 2
退出内层 -> nesting = 1，仍然保持保护
退出外层 -> nesting = 0，恢复中断
```

横向流程：

```text
enter -> nesting 1
enter -> nesting 2
exit -> nesting 1，不开中断
exit -> nesting 0，恢复中断
```

## 临界段应该尽量短

临界段会降低系统响应性。

因为在临界段期间，一部分中断被屏蔽，任务切换也可能被延后。

所以临界段原则是：

```text
只保护必须保护的共享资源访问
临界段越短越好
不要在临界段里做长延时
不要在临界段里做复杂打印
不要在临界段里等待外部事件
```

错误示例：

```c
taskENTER_CRITICAL();
Delay_ms(500);
taskEXIT_CRITICAL();
```

这会让系统在 500ms 内无法正常响应很多中断和调度事件，是非常差的做法。

## 和前面课程的关系

对应你已经学过的内容：

```text
lesson-03-exti：知道中断能打断主程序
lesson-04-systick：知道 SysTick 每 1ms 打断一次
lesson-06：知道 PendSV 可用于任务切换
lesson-07：知道某些共享数据操作不能被这些打断破坏
```

可以把它们串起来：

```text
中断/异常让系统具备实时响应能力
任务切换让系统具备多任务能力
临界段保护让共享数据在多任务和中断环境下保持一致
```

## 本课总结

这节课记住五句话：

```text
临界资源是多个任务或中断都可能访问的共享资源
临界段是访问临界资源时不能被打断的一小段代码
shared_count++ 这类读-改-写操作不是天然安全的
FreeRTOS 临界段底层依赖中断屏蔽，Cortex-M 上主要使用 BASEPRI
临界段必须尽量短，只保护真正需要保护的代码
```

下一课建议进入：

```text
Lesson 08：空闲任务与阻塞延时
```

这一课会把之前讲过的 `Delay_ms()` 和 RTOS 里的 `vTaskDelay()` 做更深入的对比。
