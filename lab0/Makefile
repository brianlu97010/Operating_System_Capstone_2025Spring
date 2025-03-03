CROSS = aarch64-linux-gnu-
CC = $(CROSS)gcc
CXX = $(CROSS)g++
LD = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy

CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMFLAGS = -Iinclude

SRCDIR = src/
CFILES = $(wildcard $(SRCDIR)*.c)
ASMFILES = $(wildcard $(SRCDIR)*.S)
OBJFILES = $(CFILES:.c=.o)
OBJFILES += $(ASMFILES:.S=.o)


all: kernel8.img

clean: 
	rm -rf *.img $(SRCDIR)*.elf $(SRCDIR)*.o

$(SRCDIR)%.o: $(SRCDIR)%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SRCDIR)%.o: $(SRCDIR)%.S
	$(CC) $(ASMFLAGS) -c $< -o $@

kernel8.img: $(SRCDIR)linker.ld $(OBJFILES)
	$(LD) -T $(SRCDIR)linker.ld $(OBJFILES) -o $(SRCDIR)kernel8.elf
	$(OBJCOPY) $(SRCDIR)kernel8.elf -O binary kernel8.img

debug: clean
debug: CFLAGS += -g 
debug: ASMFLAGS += -g
debug: kernel8.debug.img

kernel8.debug.img: $(SRCDIR)linker.ld $(OBJFILES)
	$(LD) -T $(SRCDIR)linker.ld $(OBJFILES) -o $(SRCDIR)kernel8.debug.elf
	$(OBJCOPY) $(SRCDIR)kernel8.debug.elf -O binary kernel8.debug.img

