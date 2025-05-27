copy the parent’s kernel stack and user stack data to child’s kernel stack and user stack

child’s user program share with parent’s user program

copy the parent’s trap frame data to child’s trap frame
including the 
```
struct trap_frame {
    unsigned long regs[31];     // General purpose registers x0-x30
    unsigned long sp_el0;       // Stack pointer
    unsigned long elr_el1;      // Program counter
    unsigned long spsr_el1;     // Processor state
};
```
When the child process return to user mode.
```
    ret_to_user:
        bl disable_irq_in_el1
        kernel_exit 0
```
It uses this trap frame to restore it’s status

So we need to modified the sp_el0 to let the child thread switch to user mode and can use it’s own user stack
```
    unsigned long parent_sp_offset = parent_tf->sp_el0 - (unsigned long)parent->user_stack;
    child_tf->sp_el0 = (unsigned long)child->user_stack + parent_sp_offset;
```
Then set the return value of fork system call
```
    child_tf->regs[0] = 0;  // Child returns 0
```

Set up the child thread’s cpu context
```
    memzero((unsigned long)&child->cpu_context, sizeof(struct cpu_context));
    child->cpu_context.lr = (unsigned long)ret_to_user; 
    child->cpu_context.sp = (unsigned long)child_tf;
```

This is for when the scheduler pick up the child thread, it will call `cpu_switch_to` to restore it’s cpu context :
```
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

Hence we need to modify `lr` and `sp` registers when creating child thread : 
```
    child->cpu_context.lr = (unsigned long)ret_to_user; 
    child->cpu_context.sp = (unsigned long)child_tf;
```

After `cpu_switch_to` executed, it will `ret` to `child->cpu_context.lr` i.e, `ret_to_user`
```
ret_to_user:
    bl disable_irq_in_el1
    kernel_exit 0
```
it restore the trap frame of child thread’s trap frame
```
struct trap_frame {
    unsigned long regs[31];     // Same as parent’s thread
    unsigned long sp_el0;       // child’s user stack pointer
    unsigned long elr_el1;      // Same as parent’s thread
    unsigned long spsr_el1;     // EL0t
};
```

After `eret`, it return to the `elr_el1` i.e, the user program where trigger the exception `svc` and continue executing the user program using user thread’s own user stack 

# OSC 2025 | Lab 4: Allocator
> [!IMPORTANT]
> Todo :
> [Advanced Exercise - Reserved Memory & Startup Allocation](#advanced-exercise---reserved-memory--startup-allocation)

## Basic Exercise 1: Buddy System

The buddy system is a well-known algorithm for allocating contiguous memory blocks. Our implementation:

- Manages physical memory in page frames (4KB blocks)
- Maintains free lists for different orders (where block size = 2^order * page_size)
- Supports allocation and freeing with automatic coalescing of buddy blocks
- Provides logging for visualization of memory operations

**Key functions:**
- `buddy_init`: Initializes the buddy system with a memory region
- `buddy_alloc_pages`: Allocates 2^order contiguous pages
- `buddy_free_pages`: Frees previously allocated pages

**Optimization:**
- When a larger block is split, smaller blocks are added to appropriate free lists
- When blocks are freed, the system checks if their buddy is also free to merge them

## Basic Exercise 2: Dynamic Memory Allocator

When there is a memory allocation request, round up the requested allocation size to the nearest size and check if there is any unallocated slot. If not, allocate a new page frame from the page allocator. Then, return one chunk to the caller. 

- Uses the buddy system for large allocations
- Maintains memory pools of common sizes (16, 32, 64, 128, 256, 512, 1024, 2048 bytes)
- Partitions pages into chunk slots for smaller allocations
- Intelligently returns memory to the buddy system when all chunks in a page are freed

**Key functions:**
- `dmalloc`: Allocates memory of a specified size
- `dfree`: Frees previously allocated memory
- `memory_pools_init`: Initializes the pool system

**Memory pools:**
Each memory pool:
- Manages chunks of a specific size
- Maintains a free list of available chunks
- Requests pages from the buddy system as needed

### Logging

Our implementation includes comprehensive logging to help with debugging and visualization:
- Allocation and deallocation events are logged
- Block splitting and merging operations are recorded
- Memory pool status updates are provided

## Advanced Exercise - Reserved Memory & Startup Allocation
* Design an API to reserve memory blocks.
* Implement the startup allocation.
> [!NOTE]
> * Your buddy system should still work when the memory size is large or contains memory holes. The usable memory region is from 0x00 to 0x3C000000, you can get this information from the memory node in devicetree.
> * All the usable memory should be used, and you should allocate the page frame array dynamically.
> * Reserved memory block detection is not part of the startup allocator. Call the API designed in the previous section to reserve those regions.
> * You can either get the memory information from the devicetree, or just simply hardcode it, below are the memory regions that have to be reserved:
>   1. Spin tables for multicore boot (0x0000 - 0x1000)
>   2. Kernel image in the physical memory
>   3. Initramfs
>   4. Devicetree (Optional, if you have implement it)
>   5. Your simple allocator (startup allocator)




## Usage

### Buddy System Demo

Our implementation includes a demo function that showcases the buddy system's capabilities:
- Allocates and frees different-sized memory blocks
- Displays the status of free lists
- Shows merging of buddy blocks during coalescing

### Dynamic Allocator Demo

There's also a demo for the dynamic allocator that:
- Allocates small chunks from memory pools
- Shows how pages are divided into chunks
- Demonstrates returning memory to the buddy system

### Building and Testing

To build and test our implementation:

1. Compile the code:
```bash
make clean && make
```

2. Run on QEMU or deploy to Raspberry Pi:
```bash
# For QEMU
make qemu

# For Raspberry Pi
make raspi
python3 send_kernel.py kernel8.img /dev/ttyUSB0
```

## Source Files

- `malloc.c/h`: Implementation of all memory allocators
- `list.h`: Implementation of linked list utilities
- `main.c`: Initializes allocators and runs demo

## Deadline
[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/V5oTyLCK)
