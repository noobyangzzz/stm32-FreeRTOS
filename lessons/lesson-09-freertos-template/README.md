# Lesson 09：建立 STM32 FreeRTOS 工程模板

## 本课目标

这一课是第一个真正运行 FreeRTOS 的 STM32F103VET6 工程。前面的课程已经准备好了 GPIO、USART、SysTick 和任务切换概念；这一课把 FreeRTOS v9.0.0 加入工程，用 GCC/ARM_CM3 portable 层在 WSL 中编译出可烧录固件。

本课重点不是复杂外设，而是观察 FreeRTOS 调度器如何让两个任务共享 CPU：

```text
main() -> 初始化 LED/USART -> 创建 LedTask -> 创建 UsartTask -> 启动调度器 -> 两个任务按 tick 和优先级运行
```

## 对比上一个 lesson

对比 Lesson 08，上一课还是概念层面的空闲任务、阻塞延时和任务状态；这一课开始有实际 FreeRTOS 代码。

对比之前的裸机代码课，本课新增了 FreeRTOS 目录：

```text
FreeRTOS/Source/list.c
FreeRTOS/Source/queue.c
FreeRTOS/Source/tasks.c
FreeRTOS/Source/event_groups.c
FreeRTOS/Source/portable/GCC/ARM_CM3/port.c
FreeRTOS/Source/portable/MemMang/heap_4.c
FreeRTOS/Source/include/
FreeRTOS/Source/portable/GCC/ARM_CM3/
```

这些文件的作用：

```text
tasks.c：任务创建、任务调度、任务阻塞和任务状态管理。
list.c：FreeRTOS 内部链表，用于管理就绪列表、阻塞列表等。
queue.c：队列实现，后续信号量、互斥量等机制也会依赖它。
event_groups.c：事件组实现，当前工程先加入，后续课程再使用。
port.c：Cortex-M3 相关移植层，包含 SVC、PendSV、SysTick 的底层调度逻辑。
heap_4.c：FreeRTOS 动态内存分配实现，xTaskCreate() 会从这里申请任务栈和 TCB。
FreeRTOSConfig.h：本工程对 FreeRTOS 内核的配置。
```

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印启动信息 -> xTaskCreate(LedTask) -> xTaskCreate(UsartTask) -> vTaskStartScheduler()
```

`xTaskCreate()` 创建任务，但创建后任务并不会马上按多任务方式运行。真正让 FreeRTOS 接管 CPU 的是：

```c
vTaskStartScheduler();
```

正常情况下，调度器启动后不会再回到 `main()` 后面的空循环；CPU 会在各个任务、中断和空闲任务之间切换。

## LedTask

LedTask 的横向流程：

```text
LedTask 运行 -> 翻转 LED3 蓝灯 -> 打印当前 tick -> vTaskDelay(500ms) -> 进入阻塞态 -> 500 tick 到期后重新就绪
```

关键代码：

```c
LED3_TOGGLE;
SendTaskLog("LedTask", "toggle LED3");
vTaskDelay(pdMS_TO_TICKS(500));
```

这里的 `vTaskDelay()` 不是裸机死等。它会让当前任务进入阻塞态，把 CPU 让给其他就绪任务或空闲任务。

## UsartTask

UsartTask 的横向流程：

```text
UsartTask 运行 -> 打印当前 tick -> vTaskDelay(1000ms) -> 进入阻塞态 -> 1000 tick 到期后重新就绪
```

关键代码：

```c
SendTaskLog("UsartTask", "task running");
vTaskDelay(pdMS_TO_TICKS(1000));
```

所以串口现象应该是：

```text
LedTask 大约每 500 tick 输出一次
UsartTask 大约每 1000 tick 输出一次
```

由于串口发送本身需要时间，实际日志间隔可能是 503 tick、1006 tick 这样的数值，这是正常现象。

## tick 日志如何理解

示例日志：

```text
[tick 15088] LedTask: toggle LED3
[tick 15091] UsartTask: task running
[tick 15591] LedTask: toggle LED3
[tick 16094] LedTask: toggle LED3
[tick 16097] UsartTask: task running
```

可以读出三个结论：

```text
1. LedTask 大约每 500ms 运行一次。
2. UsartTask 大约每 1000ms 运行一次。
3. 当两个任务同时就绪时，优先级更高的 LedTask 先运行。
```

本工程中任务优先级：

```c
xTaskCreate(LedTask, "led", 128, NULL, 2, NULL);
xTaskCreate(UsartTask, "usart", 160, NULL, 1, NULL);
```

FreeRTOS 中优先级数字越大，优先级越高。因此在两个任务同时就绪时，`LedTask` 会先执行，打印完成并进入阻塞态后，`UsartTask` 才继续运行。

## argument 参数

任务函数必须符合 FreeRTOS 的固定原型：

```c
void TaskFunction(void *pvParameters);
```

所以本工程中写成：

```c
static void LedTask(void *argument);
static void UsartTask(void *argument);
```

`argument` 对应 `xTaskCreate()` 的第 4 个参数。当前传入的是 `NULL`，所以任务里暂时不用它：

```c
(void)argument;
```

这句只是避免编译器提示“参数未使用”。后续如果想把 LED 闪烁周期、任务编号或配置结构传进任务，就可以使用这个参数。

## FreeRTOSConfig.h 关键配置

```text
configUSE_PREEMPTION = 1：启用抢占式调度。
configTICK_RATE_HZ = 1000：系统 tick 为 1000Hz，即 1 tick 约等于 1ms。
configTOTAL_HEAP_SIZE = 16KB：FreeRTOS 动态内存池大小。
INCLUDE_vTaskDelay = 1：启用 vTaskDelay() API。
```

这三行把 Cortex-M3 内核异常入口交给 FreeRTOS：

```c
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
```

含义：

```text
SVC：启动第一个任务。
PendSV：真正执行任务上下文切换。
SysTick：产生系统节拍，推动阻塞延时和调度判断。
```

## 本课结论

这一课完成了从裸机主循环到 FreeRTOS 多任务工程的过渡。

核心理解：

```text
裸机 while(1)：应用代码独占 CPU，延时通常意味着 CPU 空转。
FreeRTOS task：任务运行一小段后可以阻塞，让调度器把 CPU 分配给其他任务。
```

通过串口 tick 日志可以直接观察到：

```text
tick 是系统时间基准。
vTaskDelay() 会让任务进入阻塞态。
任务优先级决定多个任务同时就绪时谁先运行。
SVC、PendSV、SysTick 是 Cortex-M3 上 FreeRTOS 调度链路的关键异常。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd 'lessons/lesson-09-freertos-template/13-FreeRTOS移植工程模板'
make clean
make
```

生成产物：

```text
build/lesson-09-freertos-template.elf
build/lesson-09-freertos-template.hex
build/lesson-09-freertos-template.bin
build/lesson-09-freertos-template.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
