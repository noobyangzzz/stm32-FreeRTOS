/*********************************************************************************************/
Program: lesson-17-counting-semaphore

Target board:
Wildfire STM32F103 Guide board, STM32F103VET6.

Purpose:
Learn FreeRTOS counting semaphore based event accumulation.

Expected behavior:
1. USART1 prints "lesson-17-counting-semaphore start" after reset.
2. SignalTask gives a counting semaphore every 500 ms.
3. WaitTask takes the semaphore and simulates 1500 ms processing.
4. The semaphore count grows because production is faster than consumption.
5. When count reaches 5, further gives fail until WaitTask takes again.

Main flow:
main -> LED init -> USART init -> xSemaphoreCreateCounting(5, 0)
-> CreateTasks -> start scheduler -> SignalTask gives -> WaitTask takes.

Lesson focus:
Observe xSemaphoreCreateCounting(), max count, current count and full condition.

Build:
make clean
make

Flash:
Use build/lesson-17-counting-semaphore.bin at address 0x08000000,
or use build/lesson-17-counting-semaphore.hex directly.

/*********************************************************************************************/
