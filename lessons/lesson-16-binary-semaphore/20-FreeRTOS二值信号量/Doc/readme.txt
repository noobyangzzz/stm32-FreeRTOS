/*********************************************************************************************/
Program: lesson-16-binary-semaphore

Target board:
Wildfire STM32F103 Guide board, STM32F103VET6.

Purpose:
Learn FreeRTOS binary semaphore based task synchronization.

Expected behavior:
1. USART1 prints "lesson-16-binary-semaphore start" after reset.
2. SignalTask gives a binary semaphore every 1000 ms.
3. WaitTask blocks on xSemaphoreTake().
4. WaitTask wakes after each give, toggles LED3 and prints a log.

Main flow:
main -> LED init -> USART init -> xSemaphoreCreateBinary -> CreateTasks
-> start scheduler -> SignalTask gives semaphore -> WaitTask takes semaphore.

Lesson focus:
Observe xSemaphoreCreateBinary(), xSemaphoreGive(), xSemaphoreTake() and blocking wait.

Build:
make clean
make

Flash:
Use build/lesson-16-binary-semaphore.bin at address 0x08000000,
or use build/lesson-16-binary-semaphore.hex directly.

/*********************************************************************************************/
