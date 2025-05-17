CROSS = aarch64-linux-gnu-
CC = $(CROSS)gcc
CXX = $(CROSS)g++
LD = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy

CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMFLAGS = -Iinclude

# Build directory
SRC_BUILD_DIR = build
BOOT_BUILD_DIR = $(SRC_BUILD_DIR)/bootloader
KERNEL_BUILD_DIR = $(SRC_BUILD_DIR)/kernel

# Common
SRCDIR = src
CFILES = $(wildcard $(SRCDIR)/*.c)
ASMFILES = $(wildcard $(SRCDIR)/*.S)
COBJFILES = $(patsubst $(SRCDIR)/%.c,$(SRC_BUILD_DIR)/%.c.o,$(CFILES))
ASMOBJFILES = $(patsubst $(SRCDIR)/%.S,$(SRC_BUILD_DIR)/%.S.o,$(ASMFILES))
OBJFILES = $(COBJFILES) $(ASMOBJFILES)

# Bootloader
BOOT_SRCDIR = src/bootloader
BOOT_LINKER = $(BOOT_SRCDIR)/linker.ld
BOOT_CFILES = $(BOOT_SRCDIR)/main.c $(SRCDIR)/muart.c $(SRCDIR)/utils.c $(SRCDIR)/string.c
BOOT_ASMFILES = $(BOOT_SRCDIR)/boot.S
BOOT_OBJFILES = $(patsubst $(SRCDIR)/%.c,$(SRC_BUILD_DIR)/%.c.o,$(filter $(SRCDIR)/%,$(BOOT_CFILES)))
BOOT_OBJFILES += $(patsubst $(SRCDIR)/%.S,$(SRC_BUILD_DIR)/%.S.o,$(filter $(SRCDIR)/%,$(BOOT_ASMFILES)))

# Kernel
KERNEL_SRCDIR = src/kernel
KERNEL_CFILES = $(wildcard $(KERNEL_SRCDIR)/*.c)
KERNEL_CFILES += $(wildcard $(SRCDIR)/*.c)
KERNEL_ASMFILES = $(wildcard $(KERNEL_SRCDIR)/*.S)
KERNEL_ASMFILES += $(wildcard $(SRCDIR)/*.S)
KERNEL_OBJFILES = $(patsubst $(SRCDIR)/%.c,$(SRC_BUILD_DIR)/%.c.o,$(filter $(SRCDIR)/%,$(KERNEL_CFILES)))
KERNEL_OBJFILES += $(patsubst $(SRCDIR)/%.S,$(SRC_BUILD_DIR)/%.S.o,$(filter $(SRCDIR)/%,$(KERNEL_ASMFILES)))

all: kernel8.img bootloader.img

clean: 
	rm -rf *.img $(SRC_BUILD_DIR) $(KERNEL_SRCDIR)/*.elf $(BOOT_SRCDIR)/*.elf

# Default platform
PLATFORM ?= qemu

# Choose the platform (Raspi or QEMU)
ifeq ($(PLATFORM),raspi)
    CFLAGS += -DRASPI
endif

# Setting Platform
qemu:
	$(MAKE) PLATFORM=qemu

raspi:
	$(MAKE) PLATFORM=raspi

# Create build directories
$(SRC_BUILD_DIR):
	mkdir -p $(SRC_BUILD_DIR)

$(BOOT_BUILD_DIR): | $(SRC_BUILD_DIR)
	mkdir -p $(BOOT_BUILD_DIR)

$(KERNEL_BUILD_DIR): | $(SRC_BUILD_DIR)
	mkdir -p $(KERNEL_BUILD_DIR)

# Common
$(SRC_BUILD_DIR)/%.c.o: $(SRCDIR)/%.c | $(SRC_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_BUILD_DIR)/%.S.o: $(SRCDIR)/%.S | $(SRC_BUILD_DIR)
	$(CC) $(ASMFLAGS) -c $< -o $@

# Bootloader
$(BOOT_BUILD_DIR)/%.c.o: $(BOOT_SRCDIR)/%.c | $(BOOT_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BOOT_BUILD_DIR)/%.S.o: $(BOOT_SRCDIR)/%.S | $(BOOT_BUILD_DIR)
	$(CC) $(ASMFLAGS) -c $< -o $@

# Kernel
$(KERNEL_BUILD_DIR)/%.c.o: $(KERNEL_SRCDIR)/%.c | $(KERNEL_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BUILD_DIR)/%.S.o: $(KERNEL_SRCDIR)/%.S | $(KERNEL_BUILD_DIR)
	$(CC) $(ASMFLAGS) -c $< -o $@


# Bootloader
bootloader.img: $(BOOT_SRCDIR)/linker.ld $(BOOT_OBJFILES)
	$(LD) -T $(BOOT_SRCDIR)/linker.ld $(BOOT_OBJFILES) -o $(BOOT_SRCDIR)/bootloader.elf
	$(OBJCOPY) $(BOOT_SRCDIR)/bootloader.elf -O binary bootloader.img

# Kerenl
kernel8.img: $(KERNEL_SRCDIR)/linker.ld $(KERNEL_OBJFILES)
	$(LD) -T $(KERNEL_SRCDIR)/linker.ld $(KERNEL_OBJFILES) -o $(KERNEL_SRCDIR)/kernel8.elf
	$(OBJCOPY) $(KERNEL_SRCDIR)/kernel8.elf -O binary kernel8.img


# Debug
debug: CFLAGS += -g 
debug: ASMFLAGS += -g
debug: kernel8.debug.img bootloader.debug.img

# Bootloader
bootloader.debug.img: $(BOOT_SRCDIR)/linker.ld $(BOOT_OBJFILES)
	$(LD) -T $(BOOT_SRCDIR)/linker.ld $(BOOT_OBJFILES) -o $(BOOT_SRCDIR)/bootloader.debug.elf
	$(OBJCOPY) $(BOOT_SRCDIR)/bootloader.debug.elf -O binary bootloader.debug.img

# Kerenl
kernel8.debug.img: $(KERNEL_SRCDIR)/linker.ld $(KERNEL_OBJFILES)
	$(LD) -T $(KERNEL_SRCDIR)/linker.ld $(KERNEL_OBJFILES) -o $(KERNEL_SRCDIR)/kernel8.debug.elf
	$(OBJCOPY) $(KERNEL_SRCDIR)/kernel8.debug.elf -O binary kernel8.debug.img

target1:
	@echo $(KERNEL_OBJFILES)