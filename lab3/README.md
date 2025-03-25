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
make test1
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


## Basic Exercise 2 : Timer Interrupts
### Timer Configuration Registers

The timer is configured using several system registers:

- **`cntpct_el0`**: The timer's current count value
- **`cntp_cval_el0`**: A comparison value - when the timer count equals or exceeds this value, an interrupt is triggered
- **`cntp_tval_el0`**: A convenience register for setting the timer - it represents the difference between `cntp_cval_el0` and `cntpct_el0`
- **`cntfrq_el0`**: Contains the frequency of the system counter in Hz (ticks per second)

### Implementation Steps

1. **Enable the core timer**:
   - Set `cntp_ctl_el0` to 1 to enable the timer
   - Configure `cntp_tval_el0` to set the first timeout (2 seconds in our implementation)

2. **Enable the timer interrupt**:
   - Write to the Core 0 Timers interrupt control register (0x40000040)
   - Set bit 1 (`nCNTPNSIRQ`) to enable the Non-Secure Physical Timer interrupt

3. **Enable CPU interrupts**:
   - Configure `SPSR_EL1` to 0 when transitioning to EL0, enabling all interrupts
   - This ensures timer interrupts are only processed when running user programs at EL0

### Interrupt Handling Flow

When a timer interrupt occurs while executing at EL0:

1. The processor automatically:
   - Saves the current processor state to `SPSR_EL1`
   - Saves the return address to `ELR_EL1`
   - Jumps to the high level IRQ handler entry in the vector table (`irq_handler`)

2. The IRQ handler:
   - Saves all general-purpose registers to preserve context
   - Reads the Core 0 interrupt source register (0x40000060)
   - Checks if bit 1 is set, indicating a timer interrupt
   - If it's a timer interrupt, branches to the timer-specific handler

3. The timer-specific handler:
   - Calculates the elapsed time since boot using `cntpct_el0` and `cntfrq_el0`
   - Prints this time in seconds
   - Resets the timer for the next interrupt (2 seconds later)
   - Returns to the high-level IRQ handler

4. Then the high-level IRQ handler:
   - Restores all registers
   - Uses `eret` to return to the user program

### Timer Reset Mechanism
To set up periodic timer interrupts, we need to reset the timer in each interrupt handler:

```assembly
// Reset timer for next interrupt
mrs x0, cntfrq_el0      // Get timer frequency
lsl x0, x0, #1          // Multiply by 2 (shift left by 1)
msr cntp_tval_el0, x0   // Set timeout for 2 seconds
```

### Usage
Refer to the [basic exercise 1](#building-and-running)

### Demo
![alt text](assets/demo2.png)
![alt text](assets/demo3.png)