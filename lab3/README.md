# OSC 2025 | Lab 2 : Booting
## Exercise 1: Reboot

### Using the Kernel Shell
When connecting to the Raspberry Pi 3, you'll see the kernel shell welcome message:

```
Welcome to OSC simple shell !!!
Type help to see all available commands 
# 
```

Available commands:
```
# help
help    : print this help menu
hello   : print Hello World !
reboot  : reboot the device
mailbox : show the mailbox info
```

### Rebooting the Raspberry Pi 3
Simply run the `reboot` command to restart the Raspberry Pi 3.

### Important Notes for UART Bootloader Users
When using the UART bootloader:
- After reboot, the bootloader will wait for a new kernel to be transmitted
- Must properly close your terminal connection before sending a new kernel image if using Screen to monitor.

#### Using Minicom (Recommend)
You can also use `minicom` which often provides more reliable device handling (this methond can keep monitoring when using python script transmit the kernel image) :
```
sudo apt-get install minicom        # Install minicom
sudo minicom -D /dev/ttyUSB0 -b 115200  # Connect to Raspberry Pi
```

To exit minicom properly:
1. Press `Ctrl + A`, then `X`
2. Confirm exit
3. Run the python script to send another kernel image


#### Using Screen
If you're monitoring the Raspberry Pi with `screen`:
1. Press `Ctrl + A`, then `\`
2. Confirm by typing `y`
3. Only then run the python script to send another kernel image

This proper disconnection prevents the "Device or resource busy" error that occurs when trying to access `/dev/ttyUSB0` while it's still in use by another process.



## Exercise 2: UART Bootloader
### Steps
1. GPU executes **the first stage bootloader** from ROM on the SoC.
2. The first stage bootloader recognizes the FAT16/32 file system and **loads the second stage bootloader** `bootcode.bin` from SD card to L2 cache.
3. `bootcode.bin` initializes hardware (e.g. SDRAM) and loads `start.elf`
4. `start.elf` reads the configuration `config.txt` and `bcm2835.dtb` ( If the dtb file exists ), to load the `bootloader.img` in load address `0x60000`.
5. `bootloader.img` will load the actual kernel image `kernel8.img` in `0x80000` through UART.

### Usage
Compile:
```bash
make clean && make
```
Then, 
1. Move the `config.txt` in [src/bootloader](src/bootloader) to SD card.
2. Move the `bootloader.img` to SD card.
3. Send the kernel image to Raspi 3 b+ : 
```bash
python3 send_kernel.py kernel8.img /dev/ttyUSB0 
```
Check whether the kernel is loaded successfully : 
```bash
sudo minicom -D /dev/ttyUSB0 -b 115200 
```
or
```bash
sudo screen /dev/ttyUSB0 115200 
```

(Recommend using minicom.)

### Emualate on qemu
> Known issue : issue [#3](https://github.com/brianlu97010/Operating_System_Capstone_2025Spring/issues/3)

Compile and run:
```bash
make clean && make
```
```bash
qemu-system-aarch64 -machine raspi3b -kernel bootloader.img -serial null -serial stdio -serial pty
```

## Exercise 3: Initial Ramdisk
### Creating Test Files
First, create a test directory with some files to include in the CPIO archive:

```bash
mkdir -p rootfs
echo "File name: file1\nThis is file1." > rootfs/file1
echo "hahahaahahahahahah" > rootfs/file2.txt

# Create the CPIO archive
cd rootfs
find . | cpio -o -H newc > ../initramfs.cpio
cd ..
```

### Deploying on Raspberry Pi 3B+

1. **Configure the SD Card**:
   - Copy the CPIO archive `initramfs.cpio` to the boot partition of your SD card
   - Add this line to `config.txt` to specify the loading address and the ramfs filename:
     ```
     initramfs initramfs.cpio 0x20000000
     ```

2. **Compile the Code**:
   ```bash
   make raspi
   ```
   This sets `PLATFORM=raspi`, which applies the `-DRASPI` flag, ensuring the correct memory address (`0x20000000`) for `initramfs` which is defined in [include/cpio.h](include/cpio.h).

3. **Transfer the kerenl to Raspi through UART bootloader**
   ```bash
   python3 send_kernel.py kernel8.img /dev/ttyUSB0 
   ```

### Emulating with QEMU
1. **Compile the Code**:
   ```bash
   make qemu
   ```
   This sets `PLATFORM=qemu`, which does **not** define `-DRASPI`, using QEMU’s default memory address (`0x8000000`). which is defined in [include/cpio.h](include/cpio.h).

2. **Run the Emulation**:
   ```bash
   qemu-system-aarch64 -m 1024 -M raspi3b -serial null -serial stdio -display none -kernel kernel8.img -initrd initramfs.cpio
   ```


### Default Behavior

- Running `make` without specifying a platform defaults to qemu (`PLATFORM=qemu`).


### Shell Commands

Once the system is running, you can use these commands to interact with the CPIO archive:

- `ls` - List all files in the CPIO archive
- `cat <filename>` - View the content of a specified file

Example usage:
```
# ls
.
file2.txt
file1

# cat file1
File name: file1
This is file1.
```

## Exercise 4: Simple Allocator
### Pre-allocated Memory Pool

The memory pool is defined in the linker script (`src/kernel/linker.ld`):

```
/* Define the heap section */
. = ALIGN(0x8);
heap_begin = .;
. = . + 0x100000; /* Allocate 1MB for heap */
heap_end = .;
```

This reserves 1MB of memory for our heap, placing it after the BSS section and before the stack.
uring proper alignment and boundary checks.

### Usage Example

The allocator can be tested using the `memAlloc` shell command:

```
# memAlloc 20
Allocated memory at: 0x00081a10

# memAlloc 40
Allocated memory at: 0x00081a28
```

Notice that the simple allocator implements the 8-byte alignment to ensure proper access for 64-bit ARMv8 operations in Raspi 3b+.

### Symbol Table Analysis

Using `nm -n src/kernel/kernel8.elf`, we can observe the memory layout:

```
-----------    Heap Section    -----------
0000000000081a00 D heap_begin
0000000000181a00 D heap_end
```

This confirms our heap is positioned correctly in memory, starting at address 0x81a10 and ending at 0x181a10 (1MB size).


## Advanced Exercise 1: Bootloader Self Relocation
### Testing Self-Relocation
1. Remove or comment out the `kernel_address=0x60000` line in the `config.txt` file
2. Build and deploy the bootloader:
   ```bash
   make clean && make raspi
   ```
3. Copy `bootloader.img` to the SD card
4. Boot the Raspberry Pi and check the output to verify relocation was successful
5. Send the kernel to the relocated bootloader

### Demo
When the bootloader successfully relocates itself, you'll see output confirming the new address:
```
bootloader main function is relocated at 0x000600e8

Kernel transmit complete. Validating checksum...
Checksum valid. Jumping to kernel at 0x00080000
Welcome to OSC simple shell !!!
Type help to see all available commands 
# 
```
You can confirm this matches our expectations by examining the original and relocated addresses:
```
$ aarch64-linux-gnu-objdump -d src/bootloader/bootloader.elf

Disassembly of section .text.boot:

...
000000000008004c <relocation>:
   8004c:       58000421        ldr     x1, 800d0 <setmemtozero+0x3c>
   80050:       58000442        ldr     x2, 800d8 <setmemtozero+0x44>
   80054:       58000463        ldr     x3, 800e0 <setmemtozero+0x4c>
...


Disassembly of section .text:

00000000000800e8 <bootloader_main>:
   800e8:       a9ba7bfd        stp     x29, x30, [sp, #-96]!
   800ec:       910003fd        mov     x29, sp
   800f0:       f9000bf3        str     x19, [sp, #16]
   800f4:       940000d1        bl      80438 <muart_init>
   800f8:       90000000        adrp    x0, 80000 <_start>
....
```
The bootloader's main function was originally at address `0x800e8` and is now executing at `0x600e8`, demonstrating that:

* The bootloader was successfully relocated from `0x80000` to `0x60000`
* All code references were properly adjusted during relocation
* The relocated bootloader can successfully load the kernel to `0x80000`


## Advanced Exercise 2:
### Deploying on Raspberry Pi 3B+
1. **Configure the SD Card**:
   - Move [bcm2710-rpi-3-b-plus.dtb](https://github.com/raspberrypi/firmware/raw/master/boot/bcm2710-rpi-3-b-plus.dtb) into SD card.
   - Copy the CPIO archive `initramfs.cpio` to the boot partition of your SD card
   - Add this line to `config.txt` to specify the loading address and the ramfs filename:
     ```
     initramfs initramfs.cpio
     ```

2. **Compile the Code**:
   ```bash
   make clean && make raspi
   ```

3. **Transfer the kerenl to Raspi through UART bootloader**
   ```bash
   python3 send_kernel.py kernel8.img /dev/ttyUSB0 
   ```

### Emulating with QEMU
1. **Compile the Code**:
   ```bash
   make qemu
   ```

2. **Run the Emulation**:
   ```bash
   qemu-system-aarch64 -machine raspi3b -kernel kernel8.img -serial null -serial stdio -dtb bcm2710-rpi-3-b-plus.dtb -initrd initramfs.cpio
   ```

### Shell Commands

Once the system is running, you can use these commands to interact with the CPIO archive:

- `ls` - List all files in the CPIO archive
- `cat <filename>` - View the content of a specified file

Example usage:
```
# ls
.
file2.txt
file1

# cat file1
File name: file1
This is file1.
```
### Demo on Raspi
![demo2](assets/advanced2.png)

## Due Date
[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/AaJgSZKl)
