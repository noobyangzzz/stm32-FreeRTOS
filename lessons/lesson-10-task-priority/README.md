# Lesson 10：任务优先级与调度顺序

## 本课目标

这一课继续使用上一课已经移植好的 FreeRTOS 工程，不再新增外设。重点放在三个任务之间的调度顺序：

```text
main() -> 初始化 LED/USART -> 创建高优先级任务 -> 创建中优先级 LED 任务 -> 创建低优先级任务 -> 启动调度器 -> 观察任务按优先级和阻塞状态运行
```

本课要观察的核心现象：

```text
优先级数字越大，任务优先级越高。
任务调用 vTaskDelay() 后进入阻塞态。
任务阻塞期间不会占用 CPU。
多个任务都处于就绪态时，FreeRTOS 先运行优先级最高的任务。
```

## 对比上一个 lesson

对比 Lesson 09，FreeRTOS 移植层、LED 驱动、USART 驱动都没有变化。本节课主要修改 `User/main.c`。

`User/main.c` 的变化：

```text
上一课：创建 LedTask 和 UsartTask 两个任务。
本课：创建 HighPriorityTask、MediumLedTask、LowPriorityTask 三个任务。
```

三个任务的优先级：

```c
xTaskCreate(HighPriorityTask, "high", 160, NULL, 3, NULL);
xTaskCreate(MediumLedTask, "medium", 160, NULL, 2, NULL);
xTaskCreate(LowPriorityTask, "low", 160, NULL, 1, NULL);
```

含义：

```text
HighPriorityTask：优先级 3，最高。
MediumLedTask：优先级 2，中间，同时负责翻转蓝灯。
LowPriorityTask：优先级 1，最低。
```

FreeRTOS 的规则是：优先级数字越大，优先级越高。

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印 lesson-10-task-priority start -> xTaskCreate(HighPriorityTask, 优先级3) -> xTaskCreate(MediumLedTask, 优先级2) -> xTaskCreate(LowPriorityTask, 优先级1) -> vTaskStartScheduler()
```

`vTaskStartScheduler()` 之后，CPU 不再按裸机 `while(1)` 的方式运行，而是由 FreeRTOS 调度器决定当前执行哪个任务。

## 三个任务

`HighPriorityTask` 的横向流程：

```text
HighPriorityTask 就绪 -> 打印 tick 日志 -> vTaskDelay(1000ms) -> 进入阻塞态 -> 1000 tick 到期 -> 重新就绪
```

`MediumLedTask` 的横向流程：

```text
MediumLedTask 就绪 -> 翻转 LED3 蓝灯 -> 打印 tick 日志 -> vTaskDelay(1000ms) -> 进入阻塞态 -> 1000 tick 到期 -> 重新就绪
```

`LowPriorityTask` 的横向流程：

```text
LowPriorityTask 就绪 -> 打印 tick 日志 -> vTaskDelay(1000ms) -> 进入阻塞态 -> 1000 tick 到期 -> 重新就绪
```

## 预期串口现象

Reset 后，串口会先输出：

```text
lesson-10-task-priority start
```

随后可以看到类似日志：

```text
[tick 0] HighPriorityTask: run, then block 1000ms
[tick 3] MediumLedTask: toggle LED3, then block 1000ms
[tick 7] LowPriorityTask: run, then block 1000ms
[tick 1003] HighPriorityTask: run, then block 1000ms
[tick 1006] MediumLedTask: toggle LED3, then block 1000ms
[tick 1010] LowPriorityTask: run, then block 1000ms
```

具体 tick 数不一定完全一样，因为串口发送本身会消耗时间。应该重点观察顺序：

```text
HighPriorityTask 先运行 -> MediumLedTask 后运行 -> LowPriorityTask 最后运行
```

这个顺序说明：当多个任务都处于就绪态时，调度器优先选择优先级最高的任务。

## 为什么每个任务都要 vTaskDelay()

如果一个高优先级任务进入 `while(1)` 后一直不阻塞、不让出 CPU，那么低优先级任务就很难获得运行机会。

所以本课每个任务都执行：

```c
vTaskDelay(pdMS_TO_TICKS(1000));
```

这句话的意思不是“CPU 原地等 1000ms”，而是：

```text
当前任务主动进入阻塞态 -> FreeRTOS 把 CPU 分配给其他就绪任务 -> 1000 tick 到期后当前任务重新回到就绪态
```

## 本课结论

这一课要建立三个判断：

```text
1. FreeRTOS 任务优先级数字越大，优先级越高。
2. vTaskDelay() 会让任务进入阻塞态，不会像裸机 delay 那样占着 CPU 空转。
3. 多个任务同时就绪时，调度器先运行最高优先级任务；高优先级任务阻塞后，低优先级任务才有机会运行。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd '/home/yang/stm32-FreeRTOS/lessons/lesson-10-task-priority/14-FreeRTOS任务优先级'
make clean
make
```

生成产物：

```text
build/lesson-10-task-priority.elf
build/lesson-10-task-priority.hex
build/lesson-10-task-priority.bin
build/lesson-10-task-priority.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
