# OSC 2025 | Lab 2 : Booting
## Exercise 2 UART Bootloader
### Steps
1. GPU executes **the first stage bootloader** from ROM on the SoC.
2. The first stage bootloader recognizes the FAT16/32 file system and **loads the second stage bootloader** `bootcode.bin` from SD card to L2 cache.
3. `bootcode.bin` initializes hardware (e.g. SDRAM) and loads `start.elf`
4. `start.elf` reads the configuration `config.txt` and `bcm2835.dtb` ( If the dtb file exists ), to load the `bootloader.img` in load address `0x60000`.
5. `bootloader.img` will load the actual kernel image `kernel8.img` in `0x80000` through UART.

### Usage
Compile and run:
```bash
make clean && make run
```
Then, 
1. Move the config.txt in src/bootloader to SD card.
2. Move the bootloader.img to SD card.
3. Send the kernel image to Raspi 3 b+ : 
```bash
python3 send_kernel.py kernel8.img /dev/ttyUSB0 
```
Check whether the kernel is loaded successfully : 
```bash
sudo screen /dev/ttyUSB0 115200 
```

### Emualate on qemu
> Known issue : issue [#3](https://github.com/brianlu97010/Operating_System_Capstone_2025Spring/issues/3)

Compile and run:
```bash
make clean && make run
```
```bash
 qemu-system-aarch64 -machine raspi3b -kernel bootloader.img -serial null -seria pty
```

[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/AaJgSZKl)
