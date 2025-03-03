<<<<<<< HEAD
# Operating System Capstone (OSC) 2025 Spring

## Course Overview
This course introduces the design and implementation of operating system kernels. Through a series of hands-on labs, students will gain both conceptual understanding and practical experience.

**Hardware Platform:** Raspberry Pi 3 Model B+ (rpi3)
- Students will develop on a real machine instead of an emulator.

**Labs:** 7 + 1 labs covering various aspects of kernel design and implementation.

---

## Lab Overview

### **Lab 0: Environment Setup**
#### **Introduction**
In this lab, you will set up the development environment by installing the required toolchain and building a bootable image for rpi3.

#### **Goals**
- Set up the development environment.
- Understand cross-platform development.
- Test your rpi3.

---

### **Lab 1: Hello World**
#### **Introduction**
You will implement a simple shell using bare-metal programming. This involves setting up mini UART to enable communication between the host and rpi3.

#### **Goals**
- Practice bare-metal programming.
- Access rpi3’s peripherals.
- Set up mini UART.
- Set up mailbox.

---

### **Lab 2: Booting**
#### **Introduction**
Booting involves initializing subsystems, matching device drivers, and loading the init user program.

#### **Goals**
- Implement a bootloader that loads kernel images through UART.
- Implement a simple allocator.
- Understand initial ramdisk.
- Understand device tree.

---

### **Lab 3: Exception and Interrupt**
#### **Introduction**
Exceptions and interrupts allow the system to handle errors, provide OS services, and manage peripheral device interactions.

#### **Goals**
- Understand exception levels in Armv8-A.
- Implement exception handling.
- Handle interrupts.
- Understand how rpi3’s peripherals trigger interrupts.
- Implement a multiplexed timer.
- Handle I/O devices concurrently.

---

### **Lab 4: Allocator**
#### **Introduction**
Dynamic memory allocation is crucial for managing kernel and user process memory efficiently.

#### **Goals**
- Implement a page frame allocator.
- Implement a dynamic memory allocator.
- Implement a startup allocator.

---

### **Lab 5: Thread and User Process**
#### **Introduction**
Multitasking is a core OS feature. This lab covers thread creation, context switching, and system calls.

#### **Goals**
- Implement thread and user process creation.
- Implement scheduler and context switching.
- Understand preemption.
- Implement POSIX signals.

---

### **Lab 6: Virtual Memory**
#### **Introduction**
Virtual memory provides process isolation by allocating separate address spaces for each user process.

#### **Goals**
- Understand ARMv8-A virtual memory system architecture.
- Implement memory management for user processes.
- Implement demand paging.
- Implement copy-on-write.

---

### **Lab 7: Virtual File System**
#### **Introduction**
A virtual file system (VFS) abstracts different file systems, providing a unified interface.

#### **Goals**
- Implement a VFS interface.
- Set up a root file system.
- Implement file operations.
- Implement file system mounting and cross-filesystem lookup.
- Implement special files (e.g., UART and framebuffer).

---

## References
- **Class website:** [OSC 2025](https://people.cs.nycu.edu.tw/~ttyeh/course/2025_Spring/IOC5226/outline.html)
- **Lab website:** [Welcome to Operating System Capstone, Spring 2025!](https://nycu-caslab.github.io/OSC2025/index.html)
- https://github.com/bztsrc/raspi3-tutorial
- https://github.com/s-matyukevich/raspberry-pi-os
- https://hackmd.io/@brianlu97010/ryLGbC-51l/%2FK2_MifDQRfiuK_ZfWms-hA

=======
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
>>>>>>> dcabd43 (Upload lab0)
