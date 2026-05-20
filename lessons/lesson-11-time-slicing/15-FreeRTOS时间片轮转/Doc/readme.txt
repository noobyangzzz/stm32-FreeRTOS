/*********************************************************************************************/
Program: lesson-11-time-slicing

Target board:
Wildfire STM32F103 Guide board, STM32F103VET6.

Purpose:
Observe FreeRTOS same-priority time slicing on STM32F103.

Expected behavior:
1. USART1 prints "lesson-11-time-slicing start" after reset.
2. TaskA and TaskB have the same priority.
3. Neither task calls vTaskDelay().
4. Both tasks keep printing about once per second because time slicing lets
   same-priority ready tasks share the CPU.

Main flow:
main -> LED init -> USART init -> create TaskA -> create TaskB
-> start scheduler -> FreeRTOS time-slices same-priority ready tasks.

Lesson focus:
Observe configUSE_TIME_SLICING, ready tasks and tick-based scheduling.

Build:
make clean
make

Flash:
Use build/lesson-11-time-slicing.bin at address 0x08000000,
or use build/lesson-11-time-slicing.hex directly.

/*********************************************************************************************/
