# OSC 2025 | Lab 5: Thread and User Process
> [!IMPORTANT]
> Todo :
> Advanced Exercise 1 - POSIX Signal - 30%


## Basic Exercise 1: Thread System - 10%

### Thread Creation and Management

The thread system implements fundamental multitasking capabilities with the following components:

**Thread Creation**: Dynamic thread creation with task descriptors and dedicated stacks
- Each thread has separate kernel and user stacks (4KB each)
- Task descriptors store CPU context and thread metadata
- Threads are added to run queue upon creation

**Round-Robin Scheduler**: Fair scheduling algorithm for threads of the same priority
- `schedule()` function picks next runnable thread from circular run queue
- Current thread moved to tail if still runnable (round-robin behavior)
- Idle thread runs when no other threads are runnable

**Context Switching**: Assembly-level CPU context preservation and restoration
- Only callee-saved registers (x19-x30, sp) are saved/restored
- Uses system register `tpidr_el1` to store current thread pointer
- Context switch implemented in `cpu_switch_to()` assembly function

**Thread Lifecycle Management**:
- Threads can voluntarily yield CPU via `schedule()`
- Thread exit sets state to ZOMBIE and removes from run queue
- Idle thread recycles zombie thread resources
- PID management with bitmap allocation (range: 2-32768)

### Key Components:
- `sched.c/sched.h`: Core scheduler implementation
- `sched.S`: Assembly context switching routines  
- `pid.c/pid.h`: Process ID management with bitmap allocation


## Basic Exercise 2: User Process and System Calls - 30%

### System Call Interface

Implements POSIX-like system calls with proper argument passing between user and kernel modes:

**Required System Calls:**
- `getpid()`: Get current process ID
- `uart_read(buf, size)`: Read from UART
- `uart_write(buf, size)`: Write to UART  
- `exec(name, argv[])`: Execute program from initramfs
- `fork()`: Create child process
- `exit()`: Terminate current process
- `mbox_call(ch, mbox)`: Hardware mailbox communication
- `kill(pid)`: Terminate specified process

**System Call Convention:**
- Arguments passed in registers x0-x7
- System call number in x8
- Return value in x0
- Uses `svc #0` instruction to trap to kernel

**Trap Frame Management**: 
- Registers saved at top of kernel stack during exceptions
- Proper context preservation for EL0 ↔ EL1 transitions
- User stack pointer (sp_el0) saved/restored correctly

### Expected Result:
Fork test should work in EL0, showing:
- Parent and child processes with different PIDs
- Proper stack pointer values for each process
- Child process returns 0, parent gets child PID

## Video Player Test - 40%

### Timer-Driven Preemption

**Core Timer Integration:**
- Timer interrupt frequency set to core timer frequency >> 5
- Enables preemptive multitasking between processes
- User programs can access timer registers in EL0:
```c
uint64_t tmp;
asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
tmp |= 1;
asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
```

**User Program Execution:**
- Loads `syscall.img` from initramfs 
- Creates kernel thread that transitions to user mode
- Executes user program in EL0 with full system call support

**Interactive Features:**
- User-space shell program with command processing
- Real-time switching between shell and background tasks
- Timer interrupts enable responsive multitasking

### Expected Result:
- User program runs fluently in EL0
- Can switch between shell and child processes
- Timer-driven preemption allows typing while background tasks run
- Must demonstrate working on both QEMU and Raspberry Pi with display output

## Architecture Details

### Task Structure
```c
struct task_struct {
    struct cpu_context cpu_context;  // Saved registers
    pid_t pid;                      // Process ID  
    int state;                      // RUNNING/ZOMBIE/DEAD
    struct task_struct* parent;     // Parent process
    void* kernel_stack;             // Kernel mode stack
    void* user_stack;               // User mode stack  
    void* user_program;             // User program memory
    size_t user_program_size;       // Program size
    struct list_head list;          // Run queue linkage
    struct list_head task;          // Task list linkage
};
```

### Exception Handling
- Exception vector table handles synchronous/asynchronous exceptions
- Separate handlers for EL0 and EL1 exceptions
- Timer and UART interrupt support
- Proper privilege level transitions

## Building and Testing

### Prerequisites
- ARM64 cross-compiler toolchain
- QEMU ARM system emulation or Raspberry Pi 3 hardware
- Python 3 for kernel transmission utilities

### Build Instructions
```bash
make clean && make
```

### Running
```bash
# For QEMU testing
make qemu

# For Raspberry Pi deployment  
python3 send_kernel.py kernel8.img /dev/ttyUSB0
```


## Source Organization

### Core Components
- `sched.c/h`: Thread scheduler and process management
- `syscall.c/h`: System call implementations
- `exception.c/S`: Exception handling and privilege transitions
- `malloc.c/h`: Memory allocation subsystem
- `timer.c/S`: Core timer and interrupt handling

### Hardware Abstraction  
- `muart.c/h`: Mini UART driver
- `async_uart.c/h`: Asynchronous UART with interrupts
- `mailbox.c/h`: VideoCore mailbox interface
- `registers.h`: Hardware register definitions

### Utilities
- `cpio.c/h`: Initramfs archive handling
- `fdt.c/h`: Device tree parsing
- `string.c/h`: String manipulation functions
- `list.h`: Circular doubly-linked list implementation

## Fork System Call Implementation Details

### Fork Implementation Overview

The fork system call creates a child process by duplicating the parent process. The implementation follows these key steps:

#### Memory Management
Copy the parent's kernel stack and user stack data to child's kernel stack and user stack:
```c
// Copy the stacks
memcpy(child->kernel_stack, parent->kernel_stack, THREAD_STACK_SIZE);
memcpy(child->user_stack, parent->user_stack, THREAD_STACK_SIZE);
```

Child's user program shares with parent's user program:
```c
child->user_program = parent->user_program;  // Share user program pointer
child->user_program_size = parent->user_program_size; 
```

#### Trap Frame Management
Copy the parent's trap frame data to child's trap frame, including:
```c
struct trap_frame {
    unsigned long regs[31];     // General purpose registers x0-x30
    unsigned long sp_el0;       // Stack pointer
    unsigned long elr_el1;      // Program counter
    unsigned long spsr_el1;     // Processor state
};
```

When the child process returns to user mode:
```c
ret_to_user:
    bl disable_irq_in_el1
    kernel_exit 0
```

It uses this trap frame to restore its status. We need to modify the sp_el0 to let the child thread switch to user mode and use its own user stack:
```c
unsigned long parent_sp_offset = parent_tf->sp_el0 - (unsigned long)parent->user_stack;
child_tf->sp_el0 = (unsigned long)child->user_stack + parent_sp_offset;
```

Then set the return value of fork system call:
```c
child_tf->regs[0] = 0;  // Child returns 0
```

#### CPU Context Setup
Set up the child thread's cpu context:
```c
memzero((unsigned long)&child->cpu_context, sizeof(struct cpu_context));
child->cpu_context.lr = (unsigned long)ret_to_user; 
child->cpu_context.sp = (unsigned long)child_tf;
```

This is for when the scheduler picks up the child thread, it will call `cpu_switch_to` to restore its cpu context:
```assembly
cpu_switch_to:
    // Save the current thread's context
    stp x19, x20, [x0, 16 * 0]
    stp x21, x22, [x0, 16 * 1]
    stp x23, x24, [x0, 16 * 2]
    stp x25, x26, [x0, 16 * 3]
    stp x27, x28, [x0, 16 * 4]
    stp fp, lr, [x0, 16 * 5]
    mov x9, sp
    str x9, [x0, 16 * 6]

    // Load the next thread's context
    ldp x19, x20, [x1, 16 * 0]
    ldp x21, x22, [x1, 16 * 1]
    ldp x23, x24, [x1, 16 * 2]
    ldp x25, x26, [x1, 16 * 3]
    ldp x27, x28, [x1, 16 * 4]
    ldp fp, lr, [x1, 16 * 5]
    ldr x9, [x1, 16 * 6]
    mov sp, x9
    msr tpidr_el1, x1 
    ret
```

Hence we need to modify `lr` and `sp` registers when creating child thread:
```c
child->cpu_context.lr = (unsigned long)ret_to_user; 
child->cpu_context.sp = (unsigned long)child_tf;
```

After `cpu_switch_to` executed, it will `ret` to `child->cpu_context.lr` i.e, `ret_to_user`:
```assembly
ret_to_user:
    bl disable_irq_in_el1
    kernel_exit 0
```

It restores the trap frame of child thread's trap frame:
```c
struct trap_frame {
    unsigned long regs[31];     // Same as parent's thread
    unsigned long sp_el0;       // child's user stack pointer
    unsigned long elr_el1;      // Same as parent's thread
    unsigned long spsr_el1;     // EL0t
};
```

After `eret`, it returns to the `elr_el1` i.e, the user program where trigger the exception `svc` and continues executing the user program using user thread's own user stack.
