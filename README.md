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

