/*********************************************************************************************/
Program: lesson-09-freertos-template

Target board:
Wildfire STM32F103 Guide board, STM32F103VET6.

Purpose:
Build the first STM32F103 FreeRTOS template with WSL/GCC.

Expected behavior:
1. USART1 prints "lesson-09-freertos-template start" after reset.
2. A FreeRTOS LED task toggles LED3 about every 500 ms and prints:
   [tick xxx] LedTask: toggle LED3
3. A FreeRTOS USART task prints about every 1000 ms:
   [tick xxx] UsartTask: task running

Main flow:
main -> LED init -> USART init -> create LedTask -> create UsartTask
-> start scheduler -> FreeRTOS switches between tasks.

Lesson focus:
Observe tick, vTaskDelay(), task priority, SVC, PendSV and SysTick in the
first STM32F103 FreeRTOS template.

Build:
make clean
make

Flash:
Use build/lesson-09-freertos-template.bin at address 0x08000000,
or use build/lesson-09-freertos-template.hex directly.

/*********************************************************************************************/
