TARGET := lesson-03-exti
BUILD_DIR := build

PREFIX := arm-none-eabi-
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE := $(PREFIX)size

CPU := -mcpu=cortex-m3 -mthumb
DEFS := -DSTM32F10X_HD -DUSE_STDPERIPH_DRIVER
INCLUDES := \
  -IUser \
  -IUser/Key \
  -IUser/Led \
  -IUser/usart \
  -ILibraries/CMSIS \
  -ILibraries/FWlib/inc

CFLAGS := $(CPU) $(DEFS) $(INCLUDES) -Wall -Wextra -Wno-unused-parameter -ffunction-sections -fdata-sections -O0 -g3
ASFLAGS := $(CPU) -x assembler-with-cpp
LDFLAGS := $(CPU) -Tstm32f103vet6.ld -Wl,-Map=$(BUILD_DIR)/$(TARGET).map -Wl,--gc-sections -nostartfiles

SRCS := \
  startup_gcc.s \
  User/main.c \
  User/Key/bsp_exti.c \
  User/Led/bsp_led.c \
  User/usart/bsp_usart.c \
  User/stm32f10x_it.c \
  Libraries/CMSIS/system_stm32f10x.c \
  Libraries/FWlib/src/misc.c \
  Libraries/FWlib/src/stm32f10x_exti.c \
  Libraries/FWlib/src/stm32f10x_gpio.c \
  Libraries/FWlib/src/stm32f10x_rcc.c \
  Libraries/FWlib/src/stm32f10x_usart.c

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
	$(SIZE) $(BUILD_DIR)/$(TARGET).elf

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) stm32f103vet6.ld
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.s.o: %.s
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
