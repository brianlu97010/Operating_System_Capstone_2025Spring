# OSC 2025 | Lab 0: Environment Setup

## Cross-Platform Development
### Cross Compiler

For this lab, you need to set up a cross-compiler to build binaries for the Raspberry Pi 3 Model B+ (rpi3). The recommended cross-compiler setup is based on the tutorial from [raspi3-tutorial](https://github.com/bztsrc/raspi3-tutorial/tree/master/00_crosscompiler).

#### Installing the Cross Compiler on Ubuntu
Run the following commands to install the necessary cross-compilation tools:
```bash
sudo apt update
sudo apt-get install crossbuild-essential-arm64
sudo apt-get install gcc-aarch64-linux-gnu
sudo apt-get install g++-aarch64-linux-gnu
sudo apt-get install cpp-aarch64-linux-gnu
```

#### Verifying the Installation
Check if the cross-compiler is installed correctly by running:
```bash
$ aarch64-linux-gnu-gcc -v
```
Expected output should include details about the GCC version and target architecture:
```
COLLECT_GCC=aarch64-linux-gnu-gcc
Target: aarch64-linux-gnu
...
gcc version 11.4.0 (Ubuntu 11.4.0-1ubuntu1~22.04)
```

You can also test by compiling a simple C program:
```bash
$ aarch64-linux-gnu-gcc hello.c -o hello_arm && file hello_arm
```
Expected output:
```
hello_arm: ELF 64-bit LSB pie executable, ARM aarch64, version 1 (SYSV),
dynamically linked, interpreter /lib/ld-linux-aarch64.so.1,
BuildID[sha1]=b0a61702a0437260174695548f295aae3a955add, for GNU/Linux 3.7.0, not stripped
```

---

## QEMU Emulator
QEMU allows you to emulate an ARM environment on your host system for testing purposes.

#### Installation
Run the following command to install QEMU:
```bash
sudo apt install qemu-system-arm
```

#### Usage
For detailed documentation, visit: [QEMU Documentation](https://www.qemu.org/docs/master/index.html)

To check available options:
```bash
qemu-system-aarch64 --help | less
```

---

## Debugging with QEMU & GDB
Debugging your kernel is crucial for development. Below are the steps to debug your system using QEMU and GDB.

#### Install `gdb-multiarch`
```bash
sudo apt-get install gdb-multiarch  
```

#### Launch QEMU with Debugging Enabled
```bash
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -s -S
```
Explanation of parameters:
- `-S` : Pause the CPU at startup, waiting for a debugger connection.
- `-s` : Equivalent to `-gdb tcp::1234`, opening a GDB server on port 1234.

#### Connect GDB to QEMU
```bash
$ gdb-multiarch
(gdb) file kernel8.elf  # Load debugging symbols from kernel8.elf
(gdb) target remote :1234  # Connect to QEMU's GDB server
```

For additional debugging tips, check out: [Debugging Guide](https://www.daodaodao123.com/?p=695)

## Test on qemu
Run the makefile 
```bash
make
```
and using qemu to emulate : 
```bash
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm
```
