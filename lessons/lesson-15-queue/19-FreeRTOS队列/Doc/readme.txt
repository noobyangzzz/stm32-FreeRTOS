/*********************************************************************************************/
Program: lesson-15-queue

Target board:
Wildfire STM32F103 Guide board, STM32F103VET6.

Purpose:
Learn FreeRTOS queue-based task communication.

Expected behavior:
1. USART1 prints "lesson-15-queue start" after reset.
2. ProducerTask sends one QueueMessage_t every 1000 ms.
3. ConsumerTask blocks on xQueueReceive() and prints each received message.
4. LED3 toggles when ProducerTask sends a message successfully.

Main flow:
main -> LED init -> USART init -> xQueueCreate -> CreateTasks
-> start scheduler -> ProducerTask sends messages -> ConsumerTask receives messages.

Lesson focus:
Observe xQueueCreate(), xQueueSend(), xQueueReceive() and blocking receive.

Build:
make clean
make

Flash:
Use build/lesson-15-queue.bin at address 0x08000000,
or use build/lesson-15-queue.hex directly.

/*********************************************************************************************/
