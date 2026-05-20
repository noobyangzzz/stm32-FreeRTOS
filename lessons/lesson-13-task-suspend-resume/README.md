# Lesson 13：任务挂起与恢复

## 本课目标

上一课保存了任务句柄：

```c
static TaskHandle_t task_a_handle = NULL;
static TaskHandle_t task_b_handle = NULL;
```

这一课正式使用任务句柄控制任务：

```text
vTaskSuspend(task_a_handle)：挂起 TaskA。
vTaskResume(task_a_handle)：恢复 TaskA。
```

本课重点是理解：

```text
任务参数 pvParameters：决定任务做什么。
任务句柄 TaskHandle_t：用于后续控制某个任务。
```

## 对比上一个 lesson

对比 Lesson 12，`User/main.c` 继续保留 `TaskConfig_t` 和 `ParamTask()`。

本课新增内容：

```text
User/main.c 新增 ControlTask()。
CreateTasks() 中新增 ControlTask 创建过程。
User/FreeRTOSConfig.h 将 INCLUDE_vTaskSuspend 从 0 改为 1。
Makefile 目标名改为 lesson-13-task-suspend-resume。
```

这部分实现的功能：

```text
TaskA：每 500ms 打印并翻转蓝灯。
TaskB：每 1000ms 打印。
ControlTask：每隔几秒挂起 TaskA，再每隔几秒恢复 TaskA。
```

## FreeRTOSConfig.h 关键配置

要使用 `vTaskSuspend()` 和 `vTaskResume()`，需要打开：

```c
#define INCLUDE_vTaskSuspend 1
```

如果这个配置是 0，相关 API 不会被编译进 FreeRTOS，调用时会出现编译或链接问题。

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印 lesson-13-task-suspend-resume start -> CreateTasks() -> vTaskStartScheduler()
```

`CreateTasks()` 的横向流程：

```text
创建 TaskA -> 传入 task_a_config -> 保存 task_a_handle -> 检查结果 -> 创建 TaskB -> 传入 task_b_config -> 保存 task_b_handle -> 检查结果 -> 创建 ControlTask -> 检查结果
```

`ParamTask()` 的横向流程：

```text
ParamTask(argument) -> 取出 TaskConfig_t 配置 -> 按配置决定是否翻转 LED -> 打印日志 -> vTaskDelay(配置周期)
```

`ControlTask()` 的横向流程：

```text
ControlTask 运行 -> vTaskDelay(3000ms) -> 打印 suspend TaskA -> vTaskSuspend(task_a_handle) -> vTaskDelay(3000ms) -> 打印 resume TaskA -> vTaskResume(task_a_handle) -> 重复
```

## vTaskSuspend()

代码：

```c
vTaskSuspend(task_a_handle);
```

含义：

```text
通过 task_a_handle 找到 TaskA。
把 TaskA 从可调度状态中移走。
TaskA 被挂起后，不会因为自己的 vTaskDelay() 到期而重新运行。
```

所以挂起期间，串口里应该看不到 TaskA 日志。

## vTaskResume()

代码：

```c
vTaskResume(task_a_handle);
```

含义：

```text
通过 task_a_handle 找到 TaskA。
把 TaskA 从挂起态恢复出来。
恢复后 TaskA 重新具备被调度运行的资格。
```

所以恢复后，串口里会重新出现 TaskA 日志。

## 预期串口现象

Reset 后先输出：

```text
lesson-13-task-suspend-resume start
```

前 3 秒，TaskA 和 TaskB 正常输出：

```text
[tick 0] TaskB: parameter period 1000ms
[tick 3] TaskA: parameter period 500ms
[tick 507] TaskA: parameter period 500ms
[tick 1007] TaskB: parameter period 1000ms
```

约 3 秒时，ControlTask 挂起 TaskA：

```text
[tick 3000] ControlTask: suspend TaskA
```

随后几秒内，TaskA 日志消失，只剩 TaskB：

```text
[tick 3010] TaskB: parameter period 1000ms
[tick 4017] TaskB: parameter period 1000ms
[tick 5024] TaskB: parameter period 1000ms
```

约 6 秒时，ControlTask 恢复 TaskA：

```text
[tick 6000] ControlTask: resume TaskA
```

之后 TaskA 日志重新出现：

```text
[tick 6005] TaskA: parameter period 500ms
[tick 6010] TaskB: parameter period 1000ms
```

具体 tick 不会完全一样，因为串口打印会消耗时间。

## 本课结论

这一课要建立四个判断：

```text
1. TaskHandle_t 是控制任务用的，不是传参数用的。
2. vTaskSuspend(handle) 会让指定任务进入挂起态。
3. 被挂起的任务不会因为 vTaskDelay() 到期而自动恢复。
4. vTaskResume(handle) 会让指定任务重新具备被调度运行的资格。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd '/home/yang/stm32-FreeRTOS/lessons/lesson-13-task-suspend-resume/17-FreeRTOS任务挂起与恢复'
make clean
make
```

生成产物：

```text
build/lesson-13-task-suspend-resume.elf
build/lesson-13-task-suspend-resume.hex
build/lesson-13-task-suspend-resume.bin
build/lesson-13-task-suspend-resume.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
