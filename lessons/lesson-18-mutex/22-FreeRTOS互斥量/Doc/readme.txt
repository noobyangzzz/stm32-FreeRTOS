/*********************************************************************************************/
Program: lesson-18-mutex

Target board:
Wildfire STM32F103 Guide board, STM32F103VET6.

Purpose:
Learn FreeRTOS mutex based shared resource protection.

Expected behavior:
1. USART1 prints "lesson-18-mutex start" after reset.
2. LowTask takes the mutex first and uses the shared resource for about 3000 ms.
3. HighTask tries to take the same mutex after about 1000 ms.
4. HighTask waits until LowTask gives the mutex.
5. Only one task uses the shared resource at a time.

Main flow:
main -> LED init -> USART init -> xSemaphoreCreateMutex
-> CreateTasks -> start scheduler -> LowTask/HighTask compete for mutex.

Lesson focus:
Observe xSemaphoreCreateMutex(), ownership, mutual exclusion and priority inheritance.

Build:
make clean
make

Flash:
Use build/lesson-18-mutex.bin at address 0x08000000,
or use build/lesson-18-mutex.hex directly.

/*********************************************************************************************/
