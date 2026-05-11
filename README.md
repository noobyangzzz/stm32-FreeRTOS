# STM32F103 FreeRTOS 学习项目

这个仓库用于记录野火 STM32F103VET6 指南者开发板的学习过程。项目目标是：在 WSL Linux 下用 `arm-none-eabi-gcc` 编译固件，在 Windows 端用野火 DAP + CoFlash 烧录到开发板。

原始野火 PDF、压缩包、安装包和大体积资料保留在本机目录中，但不纳入 git 管理。

## 目标硬件

- 开发板：野火 STM32F103 指南者
- MCU：STM32F103VET6
- 内核：ARM Cortex-M3
- Flash：512 KB
- SRAM：64 KB
- 编译环境：WSL Linux
- 工具链：`arm-none-eabi-gcc`
- 烧录方式：Windows + 野火 DAP + CoFlash

## 工具链

当前已安装并验证：

```text
arm-none-eabi-gcc 10.3.1
arm-none-eabi-objcopy 2.38
arm-none-eabi-size 2.38
```

典型编译方式：

```bash
cd lessons/<lesson>/<project>
make clean
make
```

每个实验的固件会生成在对应项目的 `build/` 目录中：

```text
*.elf
*.hex
*.bin
*.map
```

`build/` 是本地编译产物，已经被 `.gitignore` 忽略；需要固件时重新编译即可。

## 当前课程

当前进度：

```text
已完成裸机阶段的 GPIO 输出、GPIO 输入、USART1、EXTI 和 SysTick。
已完成 FreeRTOS 概念阶段：裸机与 RTOS、任务切换模型、临界段、空闲任务与阻塞延时。
最近完成：Lesson 09 建立 STM32 FreeRTOS 工程模板。
Lesson 04 已在 WSL 中使用 arm-none-eabi-gcc 编译通过。
```

### Lesson 00：GPIO 输出，点亮 RGB LED

路径：

```text
lessons/lesson-00-led/12-GPIO输出—使用固件库点亮LED灯
```

这一课的作用：

- 把野火 Keil LED 示例转换成 WSL/GCC 可编译项目。
- 理解启动流程、`main()`、GPIO 输出初始化和 RGB LED 控制。
- 生成可用 CoFlash 烧录的 `lesson-00-led.hex`。

关键文件：

- `User/main.c`：应用层主循环，按顺序切换 RGB 颜色。
- `User/led/bsp_led.c`：LED GPIO 初始化。
- `User/led/bsp_led.h`：LED 引脚、亮灭、颜色宏定义。
- `Makefile`：WSL/GCC 编译入口。
- `startup_gcc.s`：GCC 启动文件。
- `stm32f103vet6.ld`：适配 STM32F103VET6 的链接脚本。

板载 RGB LED 映射：

```text
LED1 红色 -> PB5
LED2 绿色 -> PB0
LED3 蓝色 -> PB1
```

这个 RGB LED 是共阳极接法：

```text
GPIO 输出低电平 = 亮
GPIO 输出高电平 = 灭
```

### Lesson 01：GPIO 输入，按键检测

路径：

```text
lessons/lesson-01-key/13-GPIO输入—按键检测
```

这一课的作用：

- 把野火 Keil 按键轮询示例转换成 WSL/GCC 可编译项目。
- 理解 GPIO 输入模式、按键轮询、等待松手和 LED 翻转。

关键文件：

- `User/main.c`：轮询 K1/K2，并翻转对应 LED 通道。
- `User/Key/bsp_key.c`：按键 GPIO 初始化和 `Key_Scan()`。
- `User/Key/bsp_key.h`：按键引脚定义。
- `User/Led/bsp_led.c`：LED GPIO 支持。
- `User/Led/bsp_led.h`：LED 宏定义。
- `Makefile`：WSL/GCC 编译入口。

按键映射：

```text
K1 -> PA0
K2 -> PC13
```

程序行为：

```text
上电后：红灯默认点亮。
按下并松开 K1：翻转红色通道。
按下并松开 K2：翻转绿色通道。
```

注意：这一课的 `Key_Scan()` 会等待按键松开，因此长按不会一直重复触发；但它还不是严格的机械按键消抖实现。

### Lesson 02：USART1 串口通信

路径：

```text
lessons/lesson-02-usart/21-USART—串口通信/USART1接发
```

这一课的作用：

- 建立 USART1 调试输出能力。
- 使用板载 CH340 USB 转串口连接 Windows 串口助手。
- 通过蓝灯心跳确认固件运行，通过串口输出确认通信链路。

关键文件：

- `User/main.c`：初始化蓝灯和 USART1，循环输出 `USART OK`。
- `User/usart/bsp_usart.c`：USART1 GPIO、外设和接收中断初始化，发送函数实现。
- `User/usart/bsp_usart.h`：USART1、PA9/PA10、115200 8N1 等配置。
- `User/stm32f10x_it.c`：USART1 接收中断回显。
- `Makefile`：WSL/GCC 编译入口。

串口映射：

```text
USART1 TX -> PA9  -> CH340 RXD -> Windows COM
USART1 RX -> PA10 <- CH340 TXD
```

程序行为：

```text
蓝灯周期闪烁。
串口助手持续收到 USART OK。
串口助手发送字符后，开发板原样回显。
```

### Lesson 03：EXTI 外部中断

路径：

```text
lessons/lesson-03-exti/18-EXTI—外部中断
```

这一课的作用：

- 从按键轮询过渡到按键外部中断。
- 用 USART1 打印中断事件，避免只靠 RGB LED 观察。
- 理解 GPIO、EXTI、NVIC、中断向量表、IRQHandler 的关系。

关键文件：

- `User/main.c`：初始化 LED、USART1 和 EXTI，然后空循环等待中断。
- `User/Key/bsp_exti.c`：配置 K1/K2 GPIO、EXTI 线和 NVIC。
- `User/Key/bsp_exti.h`：K1/K2 引脚、中断线、中断入口定义。
- `User/stm32f10x_it.c`：EXTI0/EXTI15_10 中断服务函数，打印 `KEY1 IRQ` / `KEY2 IRQ`。
- `startup_gcc.s`：包含外设中断向量表，使 EXTI 中断能跳到正确处理函数。

程序行为：

```text
Reset 后串口打印 lesson-03-exti start。
按 K1 后串口打印 KEY1 IRQ，并翻转红色 LED 通道。
按 K2 后串口打印 KEY2 IRQ，并翻转绿色 LED 通道。
```

### Lesson 04：SysTick 系统滴答定时器

路径：

```text
lessons/lesson-04-systick/22-SysTick—系统滴答定时器
```

这一课的作用：

- 使用 Cortex-M3 内核自带的 SysTick 产生 1ms 系统节拍。
- 建立裸机阶段的毫秒计时接口。
- 用 `Delay_ms()` 替代之前不精确的空循环延时。
- 为后续 FreeRTOS 的系统 tick、任务延时和调度节拍做准备。

关键文件：

- `User/main.c`：初始化 LED、USART1、SysTick，循环翻转蓝灯并打印 tick。
- `User/SysTick/bsp_systick.c`：SysTick 初始化、tick 计数、毫秒延时实现。
- `User/SysTick/bsp_systick.h`：SysTick 模块接口。
- `User/stm32f10x_it.c`：在 `SysTick_Handler()` 中每 1ms 累加 tick。
- `User/usart/bsp_usart.c`：复用 USART1 串口输出。
- `Makefile`：WSL/GCC 编译入口。

时钟关系：

```text
外部 HSE 晶振 8MHz -> PLL x9 -> SystemCoreClock 72MHz
SysTick_Config(SystemCoreClock / 1000) -> 每 1ms 触发一次 SysTick 中断
```

程序行为：

```text
Reset 后串口打印 lesson-04-systick start。
蓝灯每 500ms 翻转一次。
串口每 500ms 打印当前 tick，数值大约每次增加 500。
```

### Lesson 05：裸机系统与多任务系统

路径：

```text
lessons/lesson-05-baremetal-vs-rtos
```

这一课的作用：

- 从裸机主循环过渡到 RTOS 思维。
- 对比轮询系统、前后台系统和多任务系统。
- 理解 `Delay_ms()` 和 `vTaskDelay()` 的本质区别。
- 建立 SysTick、PendSV、SVC 在 FreeRTOS 中的直觉认识。

本课不生成固件，不需要烧录。

### Lesson 06：任务定义与任务切换的最小模型

路径：

```text
lessons/lesson-06-task-switching-model
```

这一课的作用：

- 理解任务函数为什么通常是 `while(1)` 且不能返回。
- 理解每个任务为什么需要独立任务栈。
- 理解任务控制块 TCB 的最小作用。
- 建立上下文切换的基本链路：保存当前任务现场，再恢复下一个任务现场。
- 建立 SysTick、PendSV 和任务切换之间的关系。

本课不生成固件，不需要烧录。

### Lesson 07：临界段保护

路径：

```text
lessons/lesson-07-critical-section
```

这一课的作用：

- 理解临界资源和临界段。
- 理解共享变量的“读-改-写”为什么可能被任务切换或中断打断。
- 理解 FreeRTOS 为什么通过屏蔽中断保护临界段。
- 建立 `BASEPRI`、临界段嵌套和“临界段必须尽量短”的基本认识。

本课不生成固件，不需要烧录。

### Lesson 08：空闲任务与阻塞延时

路径：

```text
lessons/lesson-08-idle-task-and-block-delay
```

这一课的作用：

- 对比裸机 `Delay_ms()` 和 RTOS `vTaskDelay()`。
- 理解运行态、就绪态、阻塞态。
- 理解任务延时信息为什么记录在 TCB 中。
- 理解空闲任务为什么是 RTOS 的兜底任务。
- 理解 SysTick 如何推进系统时间，并让延时到期的任务重新就绪。

本课不生成固件，不需要烧录。

### Lesson 09：建立 STM32 FreeRTOS 工程模板

路径：

```text
lessons/lesson-09-freertos-template/13-FreeRTOS移植工程模板
```

这一课的作用：

- 在 STM32F103VET6 工程中加入 FreeRTOS v9.0.0 核心源码。
- 使用 FreeRTOS 官方 GCC/ARM_CM3 port 适配 WSL 编译。
- 配置 `FreeRTOSConfig.h`。
- 让 FreeRTOS 接管 `SVC_Handler`、`PendSV_Handler` 和 `SysTick_Handler`。
- 创建 LED 任务和 USART 任务，通过 tick 串口日志验证 `vTaskDelay()`、任务优先级和调度现象。

关键文件：

- `User/main.c`：初始化 LED/USART，创建 `LedTask` 和 `UsartTask`，启动 FreeRTOS 调度器。
- `User/FreeRTOSConfig.h`：配置 tick、堆大小、任务 API、中断优先级和 SVC/PendSV/SysTick 映射。
- `FreeRTOS/Source/tasks.c`：任务创建、任务阻塞、任务调度核心。
- `FreeRTOS/Source/portable/GCC/ARM_CM3/port.c`：Cortex-M3 平台相关调度入口。
- `FreeRTOS/Source/portable/MemMang/heap_4.c`：FreeRTOS 动态内存分配。
- `Makefile`：WSL/GCC 编译入口。

程序行为：

```text
Reset 后串口打印 lesson-09-freertos-template start。
LedTask 大约每 500 tick 翻转一次蓝灯，并打印 [tick xxx] LedTask: toggle LED3。
UsartTask 大约每 1000 tick 打印 [tick xxx] UsartTask: task running。
两个任务同时就绪时，优先级更高的 LedTask 先运行。
```

本课开始重新生成可烧录固件。

## 学习计划

1. 标准外设库与板级基础：
   - GPIO 输出 LED
   - GPIO 输入按键轮询
   - USART 串口
   - EXTI 外部中断
   - SysTick/定时器
2. FreeRTOS 基础概念：
   - RTOS 是什么
   - 裸机主循环和多任务的区别
   - 任务状态与调度
3. FreeRTOS 在 STM32F103 上的移植逻辑：
   - `ARM_CM3` portable 层
   - `FreeRTOSConfig.h`
   - SysTick、PendSV、SVC
4. FreeRTOS 常用 API：
   - 任务
   - 队列
   - 信号量
   - 互斥量
   - 事件组
   - 软件定时器
   - 任务通知
5. 系统能力补强：
   - heap 与 stack
   - 中断中可调用的 FreeRTOS API
   - 中断优先级规则
   - CPU 使用率统计

## 烧录流程

1. 在 WSL 中编译：

```bash
make clean
make
```

2. 在 Windows 打开 CoFlash。
3. 选择野火 DAP/CMSIS-DAP 调试器。
4. 使用 SWD 模式，目标芯片选择 STM32F103VE 或 STM32F103xE。
5. 选择当前实验生成的 `.hex` 文件并烧录。

如果 CoFlash 不能直接打开 WSL 路径，可以先把 `.hex` 复制到普通 Windows 文件夹再选择。
