.syntax unified
.cpu cortex-m3
.thumb

.global g_pfnVectors
.global Reset_Handler

.word _sidata
.word _sdata
.word _edata
.word _sbss
.word _ebss

.section .text.Reset_Handler
.weak Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
  b copy_data_check

copy_data:
  ldr r3, [r2], #4
  str r3, [r0], #4
copy_data_check:
  cmp r0, r1
  bcc copy_data

  ldr r0, =_sbss
  ldr r1, =_ebss
  movs r2, #0
  b zero_bss_check

zero_bss:
  str r2, [r0], #4
zero_bss_check:
  cmp r0, r1
  bcc zero_bss

  bl SystemInit
  bl main
  b .

.section .text.Default_Handler
.weak Default_Handler
.type Default_Handler, %function
Default_Handler:
  b .

.macro weak_handler name
  .weak \name
  .set \name, Default_Handler
.endm

weak_handler NMI_Handler
weak_handler HardFault_Handler
weak_handler MemManage_Handler
weak_handler BusFault_Handler
weak_handler UsageFault_Handler
weak_handler SVC_Handler
weak_handler DebugMon_Handler
weak_handler PendSV_Handler
weak_handler SysTick_Handler

.section .isr_vector, "a", %progbits
.type g_pfnVectors, %object
.size g_pfnVectors, .-g_pfnVectors
g_pfnVectors:
  .word _estack
  .word Reset_Handler
  .word NMI_Handler
  .word HardFault_Handler
  .word MemManage_Handler
  .word BusFault_Handler
  .word UsageFault_Handler
  .word 0
  .word 0
  .word 0
  .word 0
  .word SVC_Handler
  .word DebugMon_Handler
  .word 0
  .word PendSV_Handler
  .word SysTick_Handler
