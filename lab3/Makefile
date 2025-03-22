CROSS = aarch64-linux-gnu-
CC = $(CROSS)gcc
CXX = $(CROSS)g++
LD = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy

CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMFLAGS = -Iinclude

# Common
SRCDIR = src/
CFILES = $(wildcard $(SRCDIR)*.c)
OBJFILES = $(CFILES:.c=.o)

# Bootloader
BOOT_SRCDIR = src/bootloader/
BOOT_LINKER = $(BOOT_SRCDIR)linker.ld
BOOT_CFILES = $(wildcard $(BOOT_SRCDIR)*.c)
BOOT_CFILES += src/muart.c
BOOT_CFILES += src/utils.c
BOOT_ASMFILES = $(wildcard $(BOOT_SRCDIR)*.S)
BOOT_OBJFILES = $(BOOT_CFILES:.c=.o)
BOOT_OBJFILES += $(BOOT_ASMFILES:.S=.o)

# Kernel
KERNEL_SRCDIR = src/kernel/
KERNEL_CFILES = $(wildcard $(KERNEL_SRCDIR)*.c)
KERNEL_CFILES += $(wildcard $(SRCDIR)*.c)
KERNEL_ASMFILES = $(wildcard $(KERNEL_SRCDIR)*.S)
KERNEL_OBJFILES = $(KERNEL_CFILES:.c=.o)
KERNEL_OBJFILES += $(KERNEL_ASMFILES:.S=.o)

all: kernel8.img bootloader.img

clean: 
	rm -rf *.img $(KERNEL_SRCDIR)*.elf $(KERNEL_SRCDIR)*.o $(BOOT_SRCDIR)*.elf $(BOOT_SRCDIR)*.o $(SRCDIR)*.o

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

# Common
$(SRCDIR)%.o: $(SRCDIR)%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SRCDIR)%.o: $(SRCDIR)%.S
	$(CC) $(ASMFLAGS) -c $< -o $@

# Bootloader
$(BOOT_SRCDIR)%.o: $(BOOT_SRCDIR)%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BOOT_SRCDIR)%.o: $(BOOT_SRCDIR)%.S
	$(CC) $(ASMFLAGS) -c $< -o $@

# Kernel
$(KERNEL_SRCDIR)%.o: $(KERNEL_SRCDIR)%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_SRCDIR)%.o: $(KERNEL_SRCDIR)%.S
	$(CC) $(ASMFLAGS) -c $< -o $@


# Bootloader
bootloader.img: $(BOOT_SRCDIR)linker.ld $(BOOT_OBJFILES)
	$(LD) -T $(BOOT_SRCDIR)linker.ld $(BOOT_OBJFILES) -o $(BOOT_SRCDIR)bootloader.elf
	$(OBJCOPY) $(BOOT_SRCDIR)bootloader.elf -O binary bootloader.img

# Kerenl
kernel8.img: $(KERNEL_SRCDIR)linker.ld $(KERNEL_OBJFILES)
	$(LD) -T $(KERNEL_SRCDIR)linker.ld $(KERNEL_OBJFILES) -o $(KERNEL_SRCDIR)kernel8.elf
	$(OBJCOPY) $(KERNEL_SRCDIR)kernel8.elf -O binary kernel8.img


# Debug
debug: CFLAGS += -g 
debug: ASMFLAGS += -g
debug: kernel8.debug.img bootloader.debug.img

# Bootloader
bootloader.debug.img: $(BOOT_SRCDIR)linker.ld $(BOOT_OBJFILES)
	$(LD) -T $(BOOT_SRCDIR)linker.ld $(BOOT_OBJFILES) -o $(BOOT_SRCDIR)bootloader.debug.elf
	$(OBJCOPY) $(BOOT_SRCDIR)bootloader.debug.elf -O binary bootloader.debug.img

# Kerenl
kernel8.debug.img: $(KERNEL_SRCDIR)linker.ld $(KERNEL_OBJFILES)
	$(LD) -T $(KERNEL_SRCDIR)linker.ld $(KERNEL_OBJFILES) -o $(KERNEL_SRCDIR)kernel8.debug.elf
	$(OBJCOPY) $(KERNEL_SRCDIR)kernel8.debug.elf -O binary kernel8.debug.img
