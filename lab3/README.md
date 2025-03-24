# OSC 2025 | Lab 3 : Exception and Interrupts
## Basic Exercise 1 : Exceptions
### EL2 to EL1

When the Raspberry Pi 3 boots, the CPU starts in EL2 (Hypervisor level). We need to transition to EL1 (Kernel level) for our operating system to run properly.

This is implemented in:
- `src/kernel/boot.S`: Contains the `from_el2_to_el1` function which sets up the necessary registers and uses the `eret` instruction to transition from EL2 to EL1.

The transition works by:
1. Setting up HCR_EL2 register to indicate that EL1 should use AArch64 state
2. Setting up SPSR_EL2 with the appropriate flags for EL1h mode with interrupts disabled
3. Setting ELR_EL2 to the return address
4. Using the `eret` instruction to switch to EL1

### EL1 to EL0

After the kernel is initialized, it needs to be able to run user programs in EL0 (User level). We've implemented a method to load user programs from the initramfs and execute them at EL0.

This is implemented in:
- `src/exception.S`: Contains the `exec_user_program` function that sets up the stack for the user program and transitions to EL0
- `src/cpio.c`: Contains `cpio_exec` which finds and loads a user program from the initramfs
- `src/shell.c`: Adds an `exec` command to the shell to run user programs

The transition works by:
1. Setting up the user stack pointer (SP_EL0)
2. Setting SPSR_EL1 to indicate EL0t mode with appropriate interrupt settings
3. Setting ELR_EL1 to the start address of the user program
4. Using `eret` to transition to EL0 and begin executing the user program

### EL0 to EL1

When a user program needs to request kernel services (via system calls) or when an exception occurs, the processor needs to transition from EL0 back to EL1. This is handled by our exception mechanism.

This is implemented in:
- `src/exception.S`: Contains the exception vector table and handler that saves register context
- `src/exception.c`: Contains the C function to process exceptions and display register contents

Key components:
1. **Exception Vector Table**: Aligned at 0x800 with entries for different exception types
2. **Context Saving**: Saves all general-purpose registers to preserve the user program's state
3. **Exception Handler**: Processes different exception types, including SVC instructions (system calls)
4. **Context Restoration**: Restores all registers before returning to the user program

### Building and Running
#### Compile User Program
```bash
cd user_program
./compile_user_program.sh
```
It will put the executable of user program into rootfs/ and then create an CPIO file.

#### Emulate on qemu
```bash
make clean && make qemu
qemu-system-aarch64 -machine raspi3b -kernel kernel8.img -serial null -serial stdio -dtb bcm2710-rpi-3-b-plus.dtb -initrd initramfs.cpio
```
#### Deploy on a Raspi
```bash
make clean && make raspi
python3 send_kernel.py kernel8.img /dev/ttyUSB0 
```
#### Usage
When the system boots, you can use the `exec` command to run a user program from the initramfs. The user program will execute system calls (SVC) which will trigger exceptions and demonstrate the transition from EL0 to EL1 and back.

![demo1](assets/demo1.png)