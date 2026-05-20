/*********************************************************************************************/
Program: lesson-13-task-suspend-resume

Target board:
Wildfire STM32F103 Guide board, STM32F103VET6.

Purpose:
Learn FreeRTOS task suspend and resume using task handles.

Expected behavior:
1. USART1 prints "lesson-13-task-suspend-resume start" after reset.
2. TaskA prints every 500 ms and toggles LED3.
3. TaskB prints every 1000 ms.
4. ControlTask suspends TaskA for about 3000 ms.
5. ControlTask resumes TaskA, and TaskA logs appear again.

Main flow:
main -> LED init -> USART init -> CreateTasks -> start scheduler
-> ControlTask suspends and resumes TaskA by task handle.

Lesson focus:
Observe TaskHandle_t, vTaskSuspend() and vTaskResume().

Build:
make clean
make

Flash:
Use build/lesson-13-task-suspend-resume.bin at address 0x08000000,
or use build/lesson-13-task-suspend-resume.hex directly.

/*********************************************************************************************/
