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

## 学习计划

1. 标准外设库与板级基础：
   - GPIO 输出 LED
   - GPIO 输入按键轮询
   - EXTI 外部中断
   - USART 串口
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

## 项目专属 skill

当前项目的 Codex skill 存放在：

```text
/home/yang/.codex/skills/stm32-freertos-guide
```

它记录了开发板假设、资料映射、课程路线和“WSL 编译、Windows 烧录”的工作方式。
