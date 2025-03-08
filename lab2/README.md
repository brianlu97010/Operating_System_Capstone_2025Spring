# OSC 2025 | Lab 2 : Booting
## Exercise 1: Reboot

### Using the Kernel Shell
When you connect to the Raspberry Pi 3, you'll see the kernel shell welcome message:

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
Simply run the `reboot` command to restart your Raspberry Pi 3.

### Important Notes for UART Bootloader Users
When using the UART bootloader:
- After reboot, the bootloader will wait for a new kernel to be transmitted
- You must properly close your terminal connection before sending a new kernel image

#### Using Screen
If you're monitoring the Raspberry Pi with `screen`:
1. Press `Ctrl + A`, then `\`
2. Confirm by typing `y`
3. Only then run the python script to send another kernel image

This proper disconnection prevents the "Device or resource busy" error that occurs when trying to access `/dev/ttyUSB0` while it's still in use by another process.

#### Using Minicom (Recommand to debug)
You can also use `minicom` which often provides more reliable device handling (this methond can keep monitoring when using python script transmit the kernel image) :
```
sudo apt-get install minicom        # Install minicom
sudo minicom -D /dev/ttyUSB0 -b 115200  # Connect to Raspberry Pi
```

To exit minicom properly:
1. Press `Ctrl + A`, then `X`
2. Confirm exit
3. Run the python script to send another kernel image


## Exercise 2: UART Bootloader
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
