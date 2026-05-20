# Lesson 14：任务删除

## 本课目标

上一课讲的是任务挂起与恢复：

```text
vTaskSuspend(task_a_handle)：TaskA 暂停运行。
vTaskResume(task_a_handle)：TaskA 恢复运行。
```

这一课讲任务删除：

```text
vTaskDelete(task_a_handle)：删除 TaskA。
```

本课重点是区分：

```text
挂起：任务还存在，可以恢复。
删除：任务被移除，不能直接恢复；如果还想运行，需要重新创建。
```

## 对比上一个 lesson

对比 Lesson 13，`User/main.c` 继续保留 `TaskConfig_t`、`ParamTask()` 和任务句柄。

本课变化：

```text
ControlTask 不再挂起/恢复 TaskA。
ControlTask 等待 5 秒后删除 TaskA。
删除 TaskA 后，ControlTask 继续周期性打印系统仍在运行。
User/FreeRTOSConfig.h 将 INCLUDE_vTaskDelete 从 0 改为 1。
Makefile 目标名改为 lesson-14-task-delete。
```

这部分实现的功能：

```text
TaskA：删除前每 500ms 打印并翻转蓝灯。
TaskB：一直每 1000ms 打印。
ControlTask：5 秒后删除 TaskA，之后每 2 秒打印系统仍在运行。
```

## FreeRTOSConfig.h 关键配置

要使用 `vTaskDelete()`，需要打开：

```c
#define INCLUDE_vTaskDelete 1
```

如果这个配置是 0，`vTaskDelete()` 不会被编译进 FreeRTOS。

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印 lesson-14-task-delete start -> CreateTasks() -> vTaskStartScheduler()
```

`CreateTasks()` 的横向流程：

```text
创建 TaskA -> 保存 task_a_handle -> 创建 TaskB -> 保存 task_b_handle -> 创建 ControlTask
```

`ParamTask()` 的横向流程：

```text
ParamTask(argument) -> 取出 TaskConfig_t 配置 -> 按配置决定是否翻转 LED -> 打印日志 -> vTaskDelay(配置周期)
```

`ControlTask()` 的横向流程：

```text
ControlTask 运行 -> vTaskDelay(5000ms) -> 打印 delete TaskA -> vTaskDelete(task_a_handle) -> task_a_handle = NULL -> 每 2000ms 打印 TaskA was deleted, system still running
```

## vTaskDelete()

代码：

```c
vTaskDelete(task_a_handle);
task_a_handle = NULL;
```

含义：

```text
通过 task_a_handle 找到 TaskA。
把 TaskA 从 FreeRTOS 调度系统中删除。
TaskA 删除后不再运行，也不能用 vTaskResume() 恢复。
task_a_handle = NULL 表示应用层不再持有这个已删除任务的句柄。
```

注意：

```text
删除任务后，任务资源最终由 FreeRTOS 空闲任务清理。
所以系统中必须允许 IdleTask 获得运行机会。
```

本工程中 TaskB 和 ControlTask 都会周期性 `vTaskDelay()`，所以 IdleTask 有机会运行。

## 预期串口现象

Reset 后先输出：

```text
lesson-14-task-delete start
```

前 5 秒，TaskA 和 TaskB 正常输出：

```text
[tick 0] TaskA: parameter period 500ms
[tick 3] TaskB: parameter period 1000ms
[tick 507] TaskA: parameter period 500ms
[tick 1007] TaskB: parameter period 1000ms
```

约 5 秒时，ControlTask 删除 TaskA：

```text
[tick 5000] ControlTask: delete TaskA
```

之后 TaskA 日志永久消失，但 TaskB 和 ControlTask 继续输出：

```text
[tick 5010] TaskB: parameter period 1000ms
[tick 6013] TaskB: parameter period 1000ms
[tick 7000] ControlTask: TaskA was deleted, system still running
[tick 7016] TaskB: parameter period 1000ms
```

这说明：

```text
TaskA 被删除了。
系统没有停止。
TaskB 仍然运行。
ControlTask 仍然运行。
```

## 挂起和删除的区别

```text
vTaskSuspend()：任务进入挂起态，任务对象还存在，可以 vTaskResume() 恢复。
vTaskDelete()：任务被删除，不能直接恢复；如果还想运行，需要重新 xTaskCreate()。
```

可以这样理解：

```text
挂起：暂停。
删除：移除。
```

## 本课结论

这一课要建立四个判断：

```text
1. TaskHandle_t 可以用来删除指定任务。
2. vTaskDelete(handle) 会让指定任务从调度系统中移除。
3. 删除和挂起不是一回事；删除后不能用 vTaskResume() 恢复。
4. 删除任务后，其他任务和系统调度仍然可以继续运行。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd '/home/yang/stm32-FreeRTOS/lessons/lesson-14-task-delete/18-FreeRTOS任务删除'
make clean
make
```

生成产物：

```text
build/lesson-14-task-delete.elf
build/lesson-14-task-delete.hex
build/lesson-14-task-delete.bin
build/lesson-14-task-delete.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
