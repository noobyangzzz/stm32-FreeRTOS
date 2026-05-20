/*********************************************************************************************/
Program: lesson-14-task-delete

Target board:
Wildfire STM32F103 Guide board, STM32F103VET6.

Purpose:
Learn FreeRTOS task deletion using a task handle.

Expected behavior:
1. USART1 prints "lesson-14-task-delete start" after reset.
2. TaskA prints every 500 ms and toggles LED3 before deletion.
3. TaskB prints every 1000 ms throughout the test.
4. ControlTask deletes TaskA after about 5000 ms.
5. TaskA logs disappear permanently, while TaskB and ControlTask continue.

Main flow:
main -> LED init -> USART init -> CreateTasks -> start scheduler
-> ControlTask deletes TaskA by task handle.

Lesson focus:
Observe TaskHandle_t, vTaskDelete() and the difference between delete and suspend.

Build:
make clean
make

Flash:
Use build/lesson-14-task-delete.bin at address 0x08000000,
or use build/lesson-14-task-delete.hex directly.

/*********************************************************************************************/
