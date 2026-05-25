/*********************************************************************************************/
Program: lesson-19-event-group

Target board:
Wildfire STM32F103 Guide board, STM32F103VET6.

Purpose:
Learn FreeRTOS event group based multi-event synchronization.

Expected behavior:
1. USART1 prints "lesson-19-event-group start" after reset.
2. EventTaskA sets EVENT_BIT_A every 1000 ms.
3. EventTaskB sets EVENT_BIT_B every 1500 ms.
4. WaitTask waits until both bits are set.
5. WaitTask toggles LED3 and prints the received bit mask.

Main flow:
main -> LED init -> USART init -> xEventGroupCreate
-> CreateTasks -> start scheduler -> tasks set bits -> WaitTask waits for both bits.

Lesson focus:
Observe xEventGroupCreate(), xEventGroupSetBits() and xEventGroupWaitBits().

Build:
make clean
make

Flash:
Use build/lesson-19-event-group.bin at address 0x08000000,
or use build/lesson-19-event-group.hex directly.

/*********************************************************************************************/
