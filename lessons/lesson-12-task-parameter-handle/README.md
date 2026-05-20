# Lesson 12：任务参数与任务句柄

## 本课目标

上一课用两个独立任务函数观察同优先级时间片轮转：

```text
TimeSliceTaskA()
TimeSliceTaskB()
```

这一课开始把任务写得更像真实工程：用一个通用任务函数，通过参数决定每个任务的具体行为。

本课重点：

```text
xTaskCreate() 第 4 个参数 pvParameters：给任务传入参数。
xTaskCreate() 第 6 个参数 pxCreatedTask：保存任务句柄。
xTaskCreate() 返回值：判断任务是否创建成功。
```

## 对比上一个 lesson

对比 Lesson 11，FreeRTOS 移植层、USART、LED 驱动都不变。本节课主要修改 `User/main.c`。

上一课：

```text
User/main.c 中有两个任务函数：TimeSliceTaskA() 和 TimeSliceTaskB()。
两个任务函数逻辑相似，但名字、LED 行为和打印内容写死在函数内部。
```

本课：

```text
User/main.c 新增 TaskConfig_t 参数结构体。
User/main.c 新增 ParamTask() 通用任务函数。
User/main.c 新增 task_a_handle 和 task_b_handle 任务句柄。
User/main.c 新增 CheckCreateResult() 检查任务创建结果。
```

这部分实现的功能：

```text
TaskConfig_t：描述一个任务的名字、打印内容、运行周期和是否翻转 LED。
ParamTask()：所有任务共用同一个任务函数，从 pvParameters 取出自己的配置。
task_a_handle/task_b_handle：保存任务句柄，为后续挂起、恢复、删除任务做准备。
CheckCreateResult()：如果 xTaskCreate() 失败，就打印错误并停在死循环。
```

## xTaskCreate() 参数

本课创建 TaskA：

```c
result = xTaskCreate(ParamTask,
                     "task_a",
                     160,
                     (void *)&task_a_config,
                     2,
                     &task_a_handle);
```

横向理解：

```text
创建任务 -> 入口函数 ParamTask -> 任务名 task_a -> 栈深度 160 -> 传入 task_a_config -> 优先级 2 -> 句柄保存到 task_a_handle
```

每个参数含义：

```text
ParamTask：任务函数入口。
"task_a"：任务名，主要用于调试。
160：任务栈深度，单位是栈单元，不是字节；Cortex-M3 上约 160 * 4 = 640 字节。
(void *)&task_a_config：传给任务函数的参数。
2：任务优先级。
&task_a_handle：保存任务句柄。
```

## pvParameters 如何进入任务函数

创建任务时传入：

```c
(void *)&task_a_config
```

任务运行后，在这里接收：

```c
static void ParamTask(void *argument)
{
  const TaskConfig_t *config = (const TaskConfig_t *)argument;
}
```

链路是：

```text
xTaskCreate() 第 4 个参数 -> FreeRTOS 保存到任务控制块 -> 任务第一次运行时传给 ParamTask(argument) -> 任务内部强制转换回 TaskConfig_t *
```

注意：传给任务的参数必须长期有效。本课使用的是 `static const` 全局配置：

```c
static const TaskConfig_t task_a_config = { ... };
static const TaskConfig_t task_b_config = { ... };
```

所以任务运行期间这些参数一直有效。

## 任务句柄是什么

任务句柄可以理解成：

```text
应用层拿到的任务引用。
```

本课保存了两个句柄：

```c
static TaskHandle_t task_a_handle = NULL;
static TaskHandle_t task_b_handle = NULL;
```

现在只是保存，还没有使用。后续课程可以用它们做：

```c
vTaskSuspend(task_a_handle);
vTaskResume(task_a_handle);
vTaskDelete(task_b_handle);
```

所以本课是为下一课“任务挂起、恢复、删除”做准备。

## main.c 运行逻辑

`main()` 的横向流程：

```text
main() -> LED_GPIO_Config() -> LED_RGBOFF -> USART_Config() -> 打印 lesson-12-task-parameter-handle start -> CreateTasks() -> vTaskStartScheduler()
```

`CreateTasks()` 的横向流程：

```text
创建 TaskA -> 传入 task_a_config -> 保存 task_a_handle -> 检查创建结果 -> 创建 TaskB -> 传入 task_b_config -> 保存 task_b_handle -> 检查创建结果
```

`ParamTask()` 的横向流程：

```text
ParamTask(argument) -> argument 转为 TaskConfig_t * -> 如果配置要求翻转 LED，则翻转 LED3 -> 按配置打印日志 -> 按配置周期 vTaskDelay()
```

## 预期串口现象

Reset 后，串口会先输出：

```text
lesson-12-task-parameter-handle start
```

随后可以看到类似：

```text
[tick 0] TaskA: parameter period 500ms
[tick 3] TaskB: parameter period 1000ms
[tick 504] TaskA: parameter period 500ms
[tick 1007] TaskA: parameter period 500ms
[tick 1010] TaskB: parameter period 1000ms
```

重点观察：

```text
TaskA 和 TaskB 使用同一个 ParamTask() 函数。
TaskA 的周期来自 task_a_config：500ms。
TaskB 的周期来自 task_b_config：1000ms。
TaskA 会翻转蓝灯，TaskB 不翻转蓝灯。
```

## 实际现象分析

实际烧录后，可能看到类似日志：

```text
lesson-12-task-parameter-handle start
[tick 0] TaskB: parameter period 1000ms
[tick 3] TaskA: parameter period 500ms
[tick 507] TaskA: parameter period 500ms
[tick 1007] TaskB: parameter period 1000ms
[tick 1010] TaskA: parameter period 500ms
[tick 1514] TaskA: parameter period 500ms
[tick 2014] TaskB: parameter period 1000ms
[tick 2017] TaskA: parameter period 500ms
```

这个现象是正确的。它说明：

```text
TaskA 大约每 500 tick 输出一次。
TaskB 大约每 1000 tick 输出一次。
TaskA 和 TaskB 虽然使用同一个 ParamTask()，但拿到的是不同配置。
```

这节课主要不是看 LED 现象，而是看串口日志。串口日志用来验证：

```text
TaskA 确实拿到了 task_a_config。
TaskB 确实拿到了 task_b_config。
同一个 ParamTask() 根据不同任务参数跑出了不同逻辑。
```

## 课堂讨论记录

### CheckCreateResult() 的作用

`xTaskCreate()` 会返回任务创建结果：

```text
pdPASS：任务创建成功。
errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY：任务创建失败，常见原因是 heap 不够，无法分配 TCB 或任务栈。
```

所以代码用 `result` 接住返回值：

```c
result = xTaskCreate(...);
CheckCreateResult(result);
```

横向链路：

```text
xTaskCreate() 尝试创建任务 -> 返回创建结果 -> result 保存结果 -> CheckCreateResult(result) 判断是否成功
```

`CheckCreateResult()` 的作用：

```text
如果 result == pdPASS：说明创建成功，函数直接返回，继续创建下一个任务。
如果 result != pdPASS：说明创建失败，串口打印 xTaskCreate failed，然后停在 while(1)。
```

这样做的意义是：任务创建失败时能明确停住并报错，不会让程序带着缺失任务继续运行。

### 任务参数和任务句柄不是一回事

本课容易混淆两个参数：

```text
xTaskCreate() 第 4 个参数 pvParameters：传任务参数。
xTaskCreate() 第 6 个参数 pxCreatedTask：保存任务句柄。
```

TaskA 的任务参数是：

```c
(void *)&task_a_config
```

TaskA 的任务句柄保存位置是：

```c
&task_a_handle
```

所以准确说法是：

```text
通过 pvParameters 传任务参数。
通过 TaskHandle_t 保存任务句柄。
```

任务句柄不是用来传参数的。它是应用层保存下来的任务引用，后续可以用来挂起、恢复、删除或调整任务。

### 本质就是给任务函数传参

可以把本课类比成普通 C 函数调用：

```c
foo(&config_a);
foo(&config_b);
```

同一个 `foo()`，因为传入参数不同，所以行为不同。

FreeRTOS 任务也是类似：

```c
xTaskCreate(ParamTask, ..., &task_a_config, ...);
xTaskCreate(ParamTask, ..., &task_b_config, ...);
```

同一个 `ParamTask()`，因为传入的 `task_a_config` 和 `task_b_config` 不同，所以表现不同。

区别是：

```text
普通函数：调用后立刻执行。
FreeRTOS 任务：xTaskCreate() 只是创建任务，真正什么时候执行由调度器决定。
```

因此本课最简洁的理解是：

```text
pvParameters = 给任务函数传参。
TaskConfig_t = 本课设计出来的任务配置项。
调度器 = 决定任务什么时候获得 CPU 运行。
```

也可以总结成：

```text
配置项负责个性化任务；
调度器负责运行这些任务。
```

## 本课结论

这一课要建立四个判断：

```text
1. 同一个任务函数可以创建多个任务。
2. pvParameters 可以把不同配置传给同一个任务函数。
3. pxCreatedTask 可以把任务句柄保存出来，后续用于控制任务。
4. xTaskCreate() 的返回值应该检查，否则任务创建失败时不容易定位问题。
5. 串口日志可以验证参数传递是否生效。
```

## 编译与烧录

在 WSL 中编译：

```bash
cd '/home/yang/stm32-FreeRTOS/lessons/lesson-12-task-parameter-handle/16-FreeRTOS任务参数与句柄'
make clean
make
```

生成产物：

```text
build/lesson-12-task-parameter-handle.elf
build/lesson-12-task-parameter-handle.hex
build/lesson-12-task-parameter-handle.bin
build/lesson-12-task-parameter-handle.map
```

CoFlash 烧录 `.bin` 时地址仍然是：

```text
0x08000000
```
