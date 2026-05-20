/*********************************************************************************************/
Program: lesson-10-task-priority

Target board:
Wildfire STM32F103 Guide board, STM32F103VET6.

Purpose:
Observe FreeRTOS task priority and blocking delay on STM32F103.

Expected behavior:
1. USART1 prints "lesson-10-task-priority start" after reset.
2. HighPriorityTask prints first, then blocks for about 1000 ms.
3. MediumLedTask toggles LED3 and prints after the high-priority task blocks.
4. LowPriorityTask prints after the higher-priority tasks block.

Main flow:
main -> LED init -> USART init -> create HighPriorityTask
-> create MediumLedTask -> create LowPriorityTask -> start scheduler
-> FreeRTOS runs ready tasks by priority.

Lesson focus:
Observe priority numbers, ready state, blocked state and vTaskDelay().

Build:
make clean
make

Flash:
Use build/lesson-10-task-priority.bin at address 0x08000000,
or use build/lesson-10-task-priority.hex directly.

/*********************************************************************************************/
