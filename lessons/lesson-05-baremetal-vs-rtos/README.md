# Lesson 05: 裸机系统与多任务系统

## 本课定位

这一课开始从 STM32 裸机基础过渡到 RTOS 思维。

前面已经完成：

```text
GPIO 输出 -> GPIO 输入 -> USART1 -> EXTI -> SysTick
```

现在需要理解：为什么这些裸机能力组合到一起以后，会逐渐暴露出程序结构和实时性问题，而 RTOS 试图解决的正是这些问题。

## 对应资料

```text
FreeRTOS 内核实现与应用开发实战指南
第 5 章：裸机系统与多任务系统
```

重点阅读：

```text
5.1 裸机系统
5.1.1 轮询系统
5.1.2 前后台系统
5.2 多任务系统
```

## 三种软件模型

### 轮询系统

轮询系统的结构最接近最早的 LED 和按键课程：

```text
main() -> 初始化硬件 -> while(1) -> 做事情1 -> 做事情2 -> 做事情3 -> 重复
```

它的特点是：所有功能都在 `while(1)` 中按顺序执行。

伪代码：

```c
int main(void)
{
    HardwareInit();

    while (1)
    {
        DoSomething1();
        DoSomething2();
        DoSomething3();
    }
}
```

优点：

```text
结构简单
容易理解
适合功能少、实时性要求低的程序
```

问题：

```text
如果 DoSomething1() 执行很久，DoSomething2() 和 DoSomething3() 都要等待
如果某个外部事件在等待期间发生又消失，可能会错过
系统响应时间取决于 while(1) 跑完一圈要多久
```

对应到你已经学过的课程：

```text
lesson-01-key 的按键轮询就是典型轮询模型
程序必须主动调用 Key_Scan() 才能发现按键
如果程序长时间卡在别的函数里，按键响应就会变差
```

## 前后台系统

前后台系统是在轮询系统基础上加入中断。

横向流程：

```text
main() 后台循环执行普通任务 -> 外部事件触发中断 -> ISR 前台快速响应 -> 设置 flag -> 回到 main() -> 后台根据 flag 处理事件
```

伪代码：

```c
volatile int flag1 = 0;

int main(void)
{
    HardwareInit();

    while (1)
    {
        if (flag1)
        {
            flag1 = 0;
            DoSomething1();
        }
    }
}

void ISR1(void)
{
    flag1 = 1;
}
```

这里的“前台”指中断服务函数，“后台”指 `main()` 里的无限循环。

优点：

```text
事件响应更及时
中断可以避免关键事件被轮询遗漏
中断里只做标记，可以减少 ISR 执行时间
```

问题：

```text
事件处理仍然主要在 main() 中按顺序执行
如果后台某个处理函数耗时太久，其他事件仍然要等待
flag 多了以后，main() 会变成大量 if 判断
各功能模块之间仍然互相影响
```

对应到你已经学过的课程：

```text
lesson-03-exti 已经进入前后台系统
K1/K2 按下后，EXTI 中断能立刻响应
但是如果真正的业务处理很多，最好不要全部放在中断函数里
```

当前 lesson-03 里直接在 ISR 中翻转 LED 并打印串口，是为了观察现象。真实项目里通常更推荐：

```text
EXTI_IRQHandler() -> 设置按键事件 flag -> main() 或任务里处理事件
```

## 多任务系统

多任务系统中，事件响应仍然可以由中断完成，但事件处理不再堆在一个 `main()` 循环里，而是拆成多个任务。

横向流程：

```text
main() -> 初始化硬件 -> 初始化 RTOS -> 创建任务 -> 启动调度器 -> LED 任务/串口任务/按键任务由调度器切换执行
```

伪代码：

```c
int main(void)
{
    HardwareInit();
    RTOS_Init();

    CreateTask(LedTask);
    CreateTask(KeyTask);
    CreateTask(UsartTask);

    RTOS_Start();
}

void LedTask(void)
{
    while (1)
    {
        LedToggle();
        Delay(500);
    }
}

void KeyTask(void)
{
    while (1)
    {
        KeyProcess();
        Delay(10);
    }
}
```

任务的本质：

```text
任务就是一个独立的、通常不会返回的函数
每个任务都有自己的栈
每个任务可以有自己的优先级
任务由调度器管理，而不是由 main() 手动顺序调用
```

关键变化：

```text
裸机：main() 决定下一步执行谁
RTOS：调度器决定下一步执行哪个任务
```

## 和 lesson-04 的关系

lesson-04 现在的主流程是：

```text
进入 main() -> 初始化 LED -> 初始化 USART1 -> 初始化 SysTick -> while(1) -> 翻转蓝灯 -> 打印 tick -> Delay_ms(500) -> 重复
```

这仍然是裸机模型。

虽然 `Delay_ms(500)` 已经比空循环精确，但它仍然是阻塞式延时：

```text
Delay_ms(500) 期间，main() 什么其他业务也不主动处理
SysTick 中断仍然每 1ms 发生
但是主程序逻辑被卡在 while 等待中
```

RTOS 中对应的写法会变成类似：

```c
void LedTask(void *argument)
{
    while (1)
    {
        LED3_TOGGLE;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

两者表面都像“延时 500ms”，但含义不同：

```text
Delay_ms(500)：当前主流程阻塞等待，CPU 主要在空等
vTaskDelay(500)：当前任务进入阻塞态，调度器可以切换到其他任务运行
```

这是 RTOS 思维最重要的第一步。

## SysTick 在 RTOS 中的角色

在裸机 lesson-04 中：

```text
SysTick 每 1ms 中断一次 -> s_tick_ms++ -> Delay_ms() 判断时间是否到
```

在 FreeRTOS 中，SysTick 仍然会周期性中断，但它的职责升级了：

```text
SysTick 中断 -> FreeRTOS tick 增加 -> 检查延时任务是否到期 -> 必要时触发任务切换
```

所以 lesson-04 不是孤立的一课，它是后面理解 FreeRTOS 的基础。

## SVC、PendSV、SysTick 的直觉理解

后面移植 FreeRTOS 到 Cortex-M3 时，会经常看到三个异常：

```text
SysTick
PendSV
SVC
```

现阶段先建立直觉：

```text
SysTick：周期性系统节拍，像系统时钟
PendSV：用于延后执行任务切换，真正切换上下文常靠它完成
SVC：通过软件触发异常，常用于启动第一个任务或进入内核服务
```

后面真正读 FreeRTOS 移植层时再展开寄存器、栈和上下文保存。

## 本课总结

横向对比：

```text
轮询系统 -> main() 顺序响应事件，main() 顺序处理事件
前后台系统 -> 中断实时响应事件，main() 顺序处理事件
多任务系统 -> 中断实时响应事件，任务实时处理事件
```

从你当前学习路径看：

```text
lesson-01-key -> 轮询系统
lesson-03-exti -> 前后台系统
lesson-04-systick -> RTOS 时间基础
lesson-05 -> 理解为什么要从裸机走向多任务
```

下一课建议进入：

```text
Lesson 06：任务定义与任务切换的最小模型
```

那一课会开始接触“任务栈”和“上下文切换”的核心思想。
