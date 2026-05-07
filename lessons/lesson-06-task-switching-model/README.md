# Lesson 06: 任务定义与任务切换的最小模型

## 本课定位

这一课开始进入 RTOS 内核的核心概念。

前面 lesson-05 解决的问题是：

```text
为什么裸机主循环会发展到多任务系统
```

这一课解决的问题是：

```text
任务是什么
任务为什么需要自己的栈
任务控制块 TCB 是什么
CPU 怎么从任务 A 切换到任务 B
```

本课暂时不烧录，不移植完整 FreeRTOS。目标是先把任务切换的逻辑链路讲清楚。

## 对应资料

```text
FreeRTOS 内核实现与应用开发实战指南
第 7 章：任务的定义与任务切换的实现
```

对应配套例程：

```text
Cortex-M3/07，任务的定义与任务切换的实现
```

## 从裸机顺序执行开始

裸机里如果要让两个变量轮流翻转，程序通常是顺序写在一个 `while(1)` 里：

```c
while (1)
{
    flag1 = 1;
    delay(100);
    flag1 = 0;
    delay(100);

    flag2 = 1;
    delay(100);
    flag2 = 0;
    delay(100);
}
```

横向流程：

```text
执行 flag1 逻辑 -> 等待 -> 执行 flag2 逻辑 -> 等待 -> 重复
```

这不是多任务，只是一个主流程顺序执行两段代码。

## 任务是什么

在 RTOS 里，会把两个功能拆成两个独立函数：

```c
void Task1_Entry(void *p_arg)
{
    while (1)
    {
        flag1 = 1;
        delay(100);
        flag1 = 0;
        delay(100);
    }
}

void Task2_Entry(void *p_arg)
{
    while (1)
    {
        flag2 = 1;
        delay(100);
        flag2 = 0;
        delay(100);
    }
}
```

任务的基本特征：

```text
任务是一个函数
任务通常有一个 void * 参数
任务主体通常是 while(1)
任务一般不能 return
任务由调度器决定什么时候运行
```

为什么任务不能返回？

```text
任务不是普通子函数
任务代表一个长期存在的执行流
如果任务函数 return，CPU 不知道应该回到哪里继续执行这个任务
```

## 为什么每个任务都需要自己的栈

裸机阶段通常只有一个主栈。

主程序、函数调用、中断返回地址、局部变量，都会使用这个栈。

但多任务系统里，Task1 和 Task2 都像是“各自运行了一半又被暂停”的函数。暂停时必须保留它们各自的现场。

所以每个任务都需要自己的栈：

```c
#define TASK1_STACK_SIZE 128
#define TASK2_STACK_SIZE 128

StackType_t Task1Stack[TASK1_STACK_SIZE];
StackType_t Task2Stack[TASK2_STACK_SIZE];
```

可以这样理解：

```text
Task1Stack 保存 Task1 的函数调用现场、局部变量、寄存器快照
Task2Stack 保存 Task2 的函数调用现场、局部变量、寄存器快照
```

如果没有独立任务栈，就无法做到：

```text
Task1 暂停在某一行
Task2 运行一段时间
再回到 Task1 刚才暂停的位置继续执行
```

## 上下文是什么

上下文就是 CPU 恢复一个任务所需的最小现场。

在 Cortex-M3 上，可以先粗略理解为：

```text
通用寄存器 R0-R12
栈指针 SP
链接寄存器 LR
程序计数器 PC
程序状态寄存器 xPSR
```

其中最关键的是：

```text
PC：下一条要执行的指令地址
SP：当前任务使用的栈顶位置
LR：函数或异常返回时要去哪里
```

上下文切换不是把整个 RAM 复制一遍，只保存 CPU 继续执行所需的关键寄存器现场。

## TCB 是什么

TCB 是 Task Control Block，任务控制块。

它可以理解为任务的身份证或档案袋。

最小 TCB 至少需要保存任务当前栈顶：

```c
typedef struct tskTaskControlBlock
{
    volatile StackType_t *pxTopOfStack;
} tskTCB;
```

`pxTopOfStack` 的意义：

```text
指向该任务当前栈顶
任务被切出时，新的栈顶保存到 pxTopOfStack
任务被切入时，从 pxTopOfStack 找到它的栈并恢复现场
```

后面的 FreeRTOS 会继续在 TCB 里加入：

```text
任务名称
优先级
状态链表节点
延时链表节点
任务栈起始地址
```

但最小任务切换模型里，先抓住一个核心：

```text
调度器不是直接管理函数，而是管理 TCB
```

## 任务切换的最小链路

假设当前正在运行 Task1，现在要切换到 Task2。

横向流程：

```text
Task1 正在运行 -> 触发任务切换 -> 保存 Task1 上下文到 Task1Stack -> 更新 Task1TCB.pxTopOfStack -> 选择 Task2TCB -> 从 Task2TCB.pxTopOfStack 找到 Task2Stack -> 恢复 Task2 上下文 -> CPU 开始执行 Task2
```

更具体一点：

```text
保存当前任务现场
-> 当前 SP 写入当前任务 TCB
-> pxCurrentTCB 指向下一个任务 TCB
-> 从下一个任务 TCB 取出 SP
-> 从该 SP 指向的任务栈恢复寄存器
-> 异常返回
-> CPU 从下一个任务的 PC 位置继续执行
```

这就是任务切换的核心。

## 为什么任务切换不是普通函数调用

普通函数调用是：

```text
函数 A -> call 函数 B -> 函数 B return -> 回到函数 A 的下一行
```

任务切换是：

```text
Task1 正在执行 -> 保存 Task1 现场 -> 恢复 Task2 现场 -> CPU 去执行 Task2 上次暂停的位置
```

区别在于：

```text
普通函数调用的返回关系是固定的
任务切换恢复的是另一个执行流
```

所以任务切换不能只靠 C 函数完成，关键部分通常需要汇编操作寄存器和栈。

## 和 SysTick、PendSV 的关系

在完整 FreeRTOS 里，常见链路是：

```text
SysTick 周期中断 -> FreeRTOS tick 增加 -> 判断是否需要调度 -> 触发 PendSV -> PendSV_Handler 执行上下文切换
```

为什么不是直接在 SysTick 中切换？

可以先这样理解：

```text
SysTick 负责周期节拍
PendSV 负责真正的延后切换
```

PendSV 的优点是它可以被设置成较低优先级，让更紧急的中断先执行，等系统准备好以后再做任务切换。

## 结合一个具体场景

场景：

```text
Task1 是 LED 任务
Task2 是串口任务
当前 CPU 正在运行 Task1
```

完整链路：

```text
Task1 正在执行 LED3_TOGGLE
-> SysTick 到来
-> CPU 进入 SysTick_Handler
-> RTOS tick 加 1
-> 调度器发现应该让 Task2 运行
-> 设置 PendSV 挂起位
-> SysTick_Handler 返回
-> CPU 进入 PendSV_Handler
-> 保存 Task1 的寄存器到 Task1Stack
-> Task1 当前栈顶写入 Task1TCB.pxTopOfStack
-> pxCurrentTCB 切换为 Task2TCB
-> 从 Task2TCB.pxTopOfStack 取出 Task2 的栈顶
-> 从 Task2Stack 恢复寄存器
-> 异常返回
-> CPU 开始执行 Task2 上次暂停的位置
```

从外部看，就像：

```text
LED 任务运行一小段 -> 串口任务运行一小段 -> LED 任务继续 -> 串口任务继续
```

但实际上始终只有一个 CPU，只是切换速度很快。

## 课堂问答：三个关键理解点

### 1. 寄存器是不是“保存东西”的地方

可以先这样理解：

```text
寄存器是 CPU 内部速度最快的一小块存储位置，用来保存 CPU 当前执行所需的关键信息。
```

但要注意，它保存的不是普通长期数据，而是 CPU 正在运行时立刻要用的执行现场。

现阶段可以先按下面方式理解：

```text
R0-R12：保存普通计算数据、函数参数、中间值
SP：保存当前栈顶位置
LR：保存函数返回或异常返回相关信息
PC：保存下一条要执行的指令地址
xPSR：保存当前 CPU 状态
```

所以更准确的说法是：

```text
寄存器 = CPU 当前执行现场的一部分
```

任务切换要保存寄存器，是因为如果不保存，任务下次恢复时就不知道：

```text
刚才执行到哪里
临时计算值是什么
当前栈在哪里
下一条该执行哪条指令
```

### 2. 为什么任务现场要放在栈里

首先要区分：

```text
任务不是“用栈存储的”
任务的运行现场主要保存在栈里
```

一个任务至少包含：

```text
任务函数代码：存在 Flash 中
任务栈：存在 RAM 中
任务控制块 TCB：存在 RAM 中
```

任务现场之所以使用栈，是因为 CPU 的函数调用机制天然就是按栈工作的。

例如：

```text
main 调用 A
A 调用 B
B 调用 C
```

返回顺序一定是：

```text
C 返回 -> B 返回 -> A 返回 -> main 继续
```

这正好是栈的后进先出规则：

```text
压入 main/A/B/C 的现场 -> 先弹出 C -> 再弹出 B -> 再弹出 A
```

为什么不能用队列？

```text
队列是先进先出
函数调用和异常返回需要后进先出
```

如果用队列保存函数调用现场，恢复顺序会和真实返回顺序相反。

为什么不能用普通数组？

实际上，任务栈在 C 代码里经常就是一个数组：

```c
StackType_t Task1Stack[128];
```

关键区别是：

```text
数组是一段连续内存
栈是对这段内存的使用规则
```

也就是说：

```text
任务栈底层可以是数组，但使用方式必须遵守栈的后进先出规则
```

每个任务都要有独立栈，是因为每个任务都可能暂停在自己的函数调用链中。

如果 Task1 和 Task2 共用一个栈，就可能出现：

```text
Task1 的现场刚保存好
Task2 运行时又把同一块栈空间覆盖
Task1 下次恢复时现场已经坏了
```

所以：

```text
每个任务独立栈 = 每个任务有独立的执行现场存放区
```

### 3. 中断/异常返回如何恢复现场

对中断最简单的理解是：

```text
中断会打断当前正在执行的代码，让 CPU 优先进入中断处理函数。
```

这个理解是对的。更细一点：

```text
中断通常来自外设，例如 EXTI、USART、定时器
异常通常来自 CPU 内核机制，例如 SysTick、PendSV、SVC、HardFault
```

在 Cortex-M3 上，中断和异常的进入/返回机制很接近，所以讲任务切换时经常把它们放在一起说。

CPU 进入异常/中断时，不只是跳转到 Handler，它还会自动保存一部分现场。

例如当前正在运行 Task1，PendSV 发生：

```text
Task1 正在运行
-> PendSV 异常发生
-> CPU 自动把 R0/R1/R2/R3/R12/LR/PC/xPSR 压入当前栈
-> 当前栈就是 Task1Stack
-> CPU 根据向量表跳到 PendSV_Handler
```

这一步中，硬件自动保存的关键寄存器是：

```text
R0
R1
R2
R3
R12
LR
PC
xPSR
```

异常返回时，CPU 会从当前 SP 指向的栈里自动弹出这些寄存器。

普通异常返回链路：

```text
main/Task1 被打断
-> CPU 自动压栈保存现场
-> 执行 Handler
-> 异常返回
-> CPU 自动出栈恢复现场
-> 回到刚才被打断的位置继续执行
```

任务切换利用的正是这个机制。

普通异常返回是：

```text
从 Task1Stack 恢复 Task1 的现场
```

任务切换时，在异常返回前会做一个关键动作：

```text
把 SP 从 Task1Stack 换成 Task2Stack
```

于是异常返回时，CPU 不再从 Task1Stack 恢复，而是从 Task2Stack 恢复。

任务切换中的异常返回链路：

```text
进入 PendSV
-> 保存 Task1 还没有被硬件自动保存的寄存器
-> Task1 的 SP 存入 Task1TCB.pxTopOfStack
-> 从 Task2TCB.pxTopOfStack 取出 Task2 的 SP
-> CPU 的 SP 改成 Task2Stack
-> 异常返回
-> CPU 从 Task2Stack 自动恢复 PC/LR/xPSR 等现场
-> CPU 开始执行 Task2
```

这里最重要的一句话是：

```text
异常进入：CPU 自动保存当前现场
异常返回：CPU 自动恢复当前 SP 指向的现场
任务切换：在异常返回前改变 SP，让 CPU 恢复另一个任务的现场
```

## 本课总结

这一课要记住四句话：

```text
任务是一个长期存在、通常不返回的函数
每个任务必须有自己的栈
TCB 保存任务管理信息，最核心的是当前栈顶 pxTopOfStack
任务切换的本质是保存当前上下文，再恢复下一个任务上下文
```

从学习路径看：

```text
lesson-04：知道 SysTick 可以产生系统节拍
lesson-05：知道为什么需要多任务
lesson-06：知道多任务切换的最小机制
```

下一课建议进入：

```text
Lesson 07：临界段保护
```

因为一旦多个任务和中断都可能访问同一份数据，就必须理解什么是临界资源、为什么需要关中断或进入临界段。
