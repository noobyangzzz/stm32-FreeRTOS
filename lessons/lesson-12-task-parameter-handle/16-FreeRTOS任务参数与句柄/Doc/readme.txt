/*********************************************************************************************/
Program: lesson-12-task-parameter-handle

Target board:
Wildfire STM32F103 Guide board, STM32F103VET6.

Purpose:
Learn FreeRTOS task parameters, task handles and create-result checking.

Expected behavior:
1. USART1 prints "lesson-12-task-parameter-handle start" after reset.
2. TaskA and TaskB are created from the same ParamTask() function.
3. TaskA uses its parameter structure to print every 500 ms and toggle LED3.
4. TaskB uses its parameter structure to print every 1000 ms.
5. Task handles are saved for later suspend/resume/delete lessons.

Main flow:
main -> LED init -> USART init -> CreateTasks -> start scheduler
-> ParamTask reads pvParameters and runs with different periods.

Lesson focus:
Observe pvParameters, TaskHandle_t and xTaskCreate return values.

Build:
make clean
make

Flash:
Use build/lesson-12-task-parameter-handle.bin at address 0x08000000,
or use build/lesson-12-task-parameter-handle.hex directly.

/*********************************************************************************************/
