# OSC 2025 | Lab 6: Virtual Memory
> [!IMPORTANT]
> **Todo** :  
> Advanced Exercise 1 - Mmap  
> Advanced Exercise 2 - Page Fault Handler & Demand Paging   
> Advanced Exercise 3 - Copy on Write  

## Introduction

Virtual memory provides isolated address spaces, allowing each user process to run in its own address space without interfering with others. This lab focuses on implementing the Memory Management Unit (MMU) and setting up address spaces for both kernel and user processes to achieve process isolation.

## Background

### Key Concepts

Before enabling virtual memory, all processes and the kernel share the same physical memory space. This creates security vulnerabilities where any process can access another process's data or even kernel data:

```c
// Without virtual memory - dangerous situation
struct task_struct* task_a = dmalloc(sizeof(struct task_struct));
struct task_struct* task_b = dmalloc(sizeof(struct task_struct));

// Task A can directly access Task B's data!
void malicious_task_a() {
    *((int*)task_b->kernel_stack) = 0xDEADBEEF;  // Corrupt Task B!
    *((int*)task_b->user_program) = evil_code;   // Modify other process code!
}
```

Virtual memory solves this by giving each process its own virtual address space:

```
Process A sees: 0x00000000 - 0xFFFFFFFF
Process B sees: 0x00000000 - 0xFFFFFFFF

But maps to different physical memory:
Process A: 0x00001000 (virtual) → 0x10000000 (physical)
Process B: 0x00001000 (virtual) → 0x20000000 (physical)
```

### ARMv8-A Memory Layout

The 64-bit virtual memory system divides address space into two regions:

```
64-bit Virtual Memory Space

         高地址
    ┌──────────────────┐ 0xFFFF_FFFF_FFFF_FFFF
    │                  │
    │  TTBR1_EL1 區域  │ ← Kernel Space
    │                  │   - Kernel code/data
    │   (Kernel Space) │   - Device drivers  
    │                  │   - System call handlers
    │                  │
    └──────────────────┘ 0xFFFF_0000_0000_0000
    ┌──────────────────┐
    │                  │
    │   Invalid Region │ ← Access causes Translation Fault
    │                  │
    └──────────────────┘ 0x0001_0000_0000_0000
    ┌──────────────────┐ 0x0000_FFFF_FFFF_FFFF
    │                  │
    │  TTBR0_EL1 區域  │ ← User Space
    │                  │   - User programs
    │   (User Space)   │   - Application memory
    │                  │   - User stacks
    │                  │
    └──────────────────┘ 0x0000_0000_0000_0000
         低地址
```

### Multi-level Paging

ARMv8-A uses a 4-level page table structure with 4KB pages:

- **PGD (Page Global Directory)** ↔ ARMv8-A Level 0
- **PUD (Page Upper Directory)** ↔ ARMv8-A Level 1  
- **PMD (Page Middle Directory)** ↔ ARMv8-A Level 2
- **PTE (Page Table Entry)** ↔ ARMv8-A Level 3

```
Virtual Address Translation Process:
┌─────────────────────────────────────────────────────────────────┐
│ PGD Index │ PUD Index │ PMD Index │ PTE Index │ Page Offset │
│ [47:39]   │ [38:30]   │ [29:21]   │ [20:12]   │ [11:0]      │
└─────────────────────────────────────────────────────────────────┘

TTBR0_EL1 → PGD → PUD → PMD → PTE → Physical Page
```

### Page Descriptors

Each page table entry is a 64-bit descriptor combining address and attributes:

```
Descriptor Format:
┌─────────────────┬─────────────────┬─────────────────┬───┬───┐
│ Upper Attrs     │ Physical Addr   │ Lower Attrs     │   │ V │
│ [63:48]         │ [47:12]         │ [11:2]          │[1]│[0]│
└─────────────────┴─────────────────┴─────────────────┴───┴───┘
```

Key attribute bits:
- **Bits[54]**: Unprivileged execute-never (EL0)
- **Bits[53]**: Privileged execute-never (EL1)  
- **Bits[47:n]**: Physical address (page-aligned)
- **Bits[10]**: Access flag (triggers fault if 0)
- **Bits[7]**: Read-only (1) vs Read-write (0)
- **Bits[6]**: User accessible (1) vs Kernel only (0)
- **Bits[4:2]**: Index to MAIR register
- **Bits[1:0]**: Entry type (11=Table/Page, 01=Block, 00=Invalid)

### Memory Attributes (MAIR)

Two memory types are used in this lab:

```c
#define MT_DEVICE_nGnRnE             0x0
#define MT_NORMAL_NC                 0x1
#define MT_DEVICE_nGnRnE_FLAGS       0x00  // Device memory nGnRnE
#define MT_NORMAL_NC_FLAGS           0x44  // Normal memory without cache

MAIR_EL1 = 0x0000_0000_0000_4400
```

- **Device Memory (nGnRnE)**: For peripherals, most restricted access
- **Normal Memory (No Cache)**: For RAM, allows reordering but no cache

## Basic Exercise 1 - Virtual Memory in Kernel Space (10%)

### Step 1: MMU Configuration

#### Translation Control Register (TCR)
Configure 48-bit address space with 4KB pages:

```c
#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

ldr x0, =TCR_CONFIG_DEFAULT
msr tcr_el1, x0
```

#### Memory Attribute Indirection Register (MAIR)
Set up memory attributes for device and normal memory:

```c
#define MAIR_VALUE (MT_DEVICE_nGnRnE_FLAGS << (8 * MT_DEVICE_nGnRnE)) | \
                   (MT_NORMAL_NC_FLAGS << (8 * MT_NORMAL_NC))

ldr x0, =MAIR_VALUE
msr mair_el1, x0
```

### Step 2: Identity Paging

Set up initial identity mapping with 2-level translation (1GB blocks):

```assembly
mov x0, 0          // PGD at physical address 0x0000
mov x1, 0x1000     // PUD at physical address 0x1000

// Clear page tables
mov x2, #0x2000    // Clear 8KB (2 pages)
bl clear_page_tables

// Setup PGD entry pointing to PUD
ldr x2, =BOOT_PGD_ATTR
orr x2, x1, x2     // Combine PUD address with attributes
str x2, [x0]       // Store to PGD[0]

// Setup PUD entries for 2GB of memory
ldr x2, =BOOT_PUD_ATTR
mov x3, 0x00000000
orr x3, x2, x3
str x3, [x1]       // 1st 1GB: 0x00000000-0x3FFFFFFF

mov x3, 0x40000000
orr x3, x2, x3
str x3, [x1, 8]    // 2nd 1GB: 0x40000000-0x7FFFFFFF

msr ttbr0_el1, x0  // Load PGD to translation base register
```

### Step 3: Map Kernel Space

Modify linker script to place kernel in upper address space:

```ld
SECTIONS
{
    . = 0xffff000000000000;  // Kernel space
    . += 0x80000;            // Kernel load address
    _kernel_start = . ;
    // ... rest of sections
}
```

Enable MMU and jump to virtual address:

```assembly
msr ttbr0_el1, x0
msr ttbr1_el1, x0  // Same PGD for both user and kernel initially
mrs x2, sctlr_el1
orr x2, x2, 1
msr sctlr_el1, x2  // Enable MMU

ldr x2, =boot_rest // Jump to virtual address
br x2

boot_rest:
    // Continue kernel initialization...
```

### Step 4: Finer Granularity Paging

Implement 3-level translation (2MB sections) for better memory management:

Physical Memory Layout:
- **First 1GB (0x00000000-0x3FFFFFFF)**:
  - RAM region (0x00000000-0x3EFFFFFF, 1008MB): Normal Memory
  - BCM2837 Peripherals (0x3F000000-0x3FFFFFFF, 16MB): Device Memory
- **Second 1GB (0x40000000-0x7FFFFFFF)**:
  - ARM Local peripherals: Device Memory

Each page table has 512 entries, enabling more precise memory attribute control.

## Basic Exercise 2 - Virtual Memory in User Space (30%)

### Data Structures

* `struct cpu_text` 新增一個 `phy_addr_pgd` 用來放 PGD 的 PA，為了在 context switch 後 load 到 TTBR0_EL1
 * 這個 offset 很重要，對應到 `struct cpu_text` 裡的相對位置
```arm
    // Load the new thread's PGD physical address
    ldr x2, [x1, 16 * 6 + 8]

    // memory barriers to guarantee previous instructions are finished. 
    dsb ish             // ensure write has completed
    msr ttbr0_el1, x2   // switch translation based address.

    // a TLB invalidation is needed because the old values are staled.
    tlbi vmalle1is      // invalidate all TLB entries
    dsb ish             // ensure completion of TLB invalidatation
    isb   
```
* `struct task_struct` 新增一個 `unsigned long* pgd` 用來 store 每個 task 的 PGD 的 VA，將 PA mapping 到 user mode 的 VA 時會用到  (作為 `mappages()` 的參數
```c
struct cpu_context {
    // ... existing registers
    unsigned long phy_addr_pgd;  // Physical address of PGD for TTBR0_EL1
};

struct task_struct {
    // ... existing fields  
    unsigned long* pgd;          // Virtual address of PGD for kernel access
};

// Allocate PGD page table for new task
new_task->pgd = dmalloc(PAGE_SIZE);
if (!new_task->pgd) {
    // Handle allocation failure
}

// Initialize PGD to zero
memzero((unsigned long)new_task->pgd, PAGE_SIZE);

// Store physical address for context switching
new_task->cpu_context.phy_addr_pgd = (unsigned long)VIRT_TO_PHYS(new_task->pgd);
```



### Memory Mapping Function

Implement `mappages()` for 4-level translation:

```c
// Extract page table indices from the input virtual address
#define PGD_IDX(va) (((va) >> 39) & 0x1FF)
#define PUD_IDX(va) (((va) >> 30) & 0x1FF) 
#define PMD_IDX(va) (((va) >> 21) & 0x1FF)
#define PTE_IDX(va) (((va) >> 12) & 0x1FF)

// Walk page tables, allocating missing levels
pte *walk(pagetable pagetable, uint64_t va, int alloc) {
    for (int level = 0; level < 3; level++) {
        uint64_t idx;
        switch (level) {
            case 0: idx = PGD_IDX(va); break;  // PGD level
            case 1: idx = PUD_IDX(va); break;  // PUD level  
            case 2: idx = PMD_IDX(va); break;  // PMD level
        }
        
        pte *pte = &pagetable[idx];
        if (*pte & PD_VALID) {
            // Entry exists, get next level table
            pagetable = (pagetable)PHYS_TO_VIRT(PTE_ADDR(*pte));
        } else {
            if (!alloc) return NULL;
            
            // Allocate new page table
            pagetable = dmalloc(PAGE_SIZE);
            memzero((unsigned long)pagetable, PAGE_SIZE);
            
            // Create table descriptor
            *pte = VIRT_TO_PHYS(pagetable) | TABLE_DESCRIPTOR;
        }
    }
    
    // Return PTE address
    return &pagetable[PTE_IDX(va)];
}
```
從 PGD 開始，如果這個 index 的 entry 的 valid bit (Bits[0]) 是 0 的話，allocate 一個 PUD page table 給他，並把這個 PUD page table 的 PA 與 TABLE_Descriptor (Bits[1:0] = 11) 合併存到 PGD page table 的 entry 中，接著繼續
檢查剛剛 allocate 的 PUD table 的 PUD_IDX 的 valid bit ...

一直到最後，return PTE page table entry 的 address
Create PTE 的 entry : 
```c
// Create PTE entry, combine physical address with attributes
*pte = current_pa | USER_PTE_ATTR;
```
PTE attribute configuration : (Refer to [hackmd note](https://hackmd.io/@brianlu97010/ryLGbC-51l/%2F_hTCCuhES22F59jdemJuBw#Format))
* Bits[10] = 1
* Bits[6] = 1
* Bits[3:2] = 01
* Bits[1:0] = 11


### User Memory Layout
> 只要是 VA = 0x0000_0000_0000_0000 ~ 0x0000_FFFF_FFFF_FFFF ，ARM v8-A 會自動將它視為在 user mode access 的 memory address，會使用 TTBR0_EL1 作為 PGD (TTBR0_EL1 是 context switch 後每個 task 各自有的 page table address，這樣才能讓每個 user process 從它們的角度來看都是有完整 0x0 ~ 0xfffffffffffff 的 memory space)
> 反之，只要是 VA = 0xFFFF_0000_0000_0000 ~ 0xFFFF_FFFF_FFFF_FFFF，ARM v8-A 會自動將它視為在 kernel mode access 的 memory address，會使用 TTBR1_EL1 作為 PGD (這邊是在 boot.S 就 defined 好了)

Map user process memory regions:

```c
#define USER_CODE_BASE     0x0000000000000000      // Program code
#define USER_STACK_BASE    0x0000ffffffffb000      // Stack start  
#define USER_STACK_TOP     0x0000fffffffff000      // Stack top
#define USER_STACK_SIZE    0x4000                  // 16KB stack
```

#### 讓 kernel thread switch 到 user mode，並執行 cpio archive `initramfs.cpio` 中的 user program :

1. 用 `kernel_thread()` create 一個 kernel thread 並執行 `kernel_fork_process_cpio()`
    * create 一個 task 時，也需要 create 一塊 page table 給他
    ```c
    // Allocate a memory space for the task's user PGD page table
    new_task->pgd = dmalloc(PAGE_SIZE);
    if (!new_task->pgd) {
        pid_free(new_task->pid);
        dfree(new_task->kernel_stack);
        dfree(new_task);
        enable_irq_in_el1();
    }
    // Initialze the content in PGD to zero
    memzero((unsigned long)new_task->pgd, PAGE_SIZE);

    // Store the physical address of the PGD page table into the cpu context
    new_task->cpu_context.phy_addr_pgd = (unsigned long)VIRT_TO_PHYS(new_task->pgd);
    ```
2. `kernel_fork_process_cpio()` 會執行 `cpio_load_program()` 以及 `move_to_user_mode`

* `cpio_load_program()` 會從 cpio archive 中找到對應的 user program
  1.  allocate 一塊 physical memory space 放 user program 的 data，然後將這塊 physical memory space mapping 到 `USER_CODE_BASE 0x0`
  2.  allocate 一塊 physical memory space 放 user mode 的 stack， 然後將這塊 physical memory space mapping 到 `USER_STACK_BASE 0x0000ffffffffb000`

```c
    // Allocate physical memory space for user program
    current->user_program = dmalloc(program_size);
    if (!current->user_program) {
        muart_puts("Error: Failed to allocate memory for user program\r\n");
        return 0;
    }

    // Copy program from initramfs to allocated memory
    memcpy(current->user_program, program_start_addr, program_size);

    // Mapping the physical memory space of the user program to the user virtual address
    unsigned long user_program_pa = (unsigned long)VIRT_TO_PHYS(current->user_program);
    if (mappages(current->pgd, USER_CODE_BASE, program_size, user_program_pa) != 0) {
        dfree(current->user_program);
        muart_puts("Error: Failed to map user program to virtual address space\r\n");
        return 0;
    }

    // Allocate a 16KB memory space for the task's user stack
    current->user_stack = dmalloc(USER_STACK_SIZE);
    if (!current->user_stack) {
        muart_puts("Failed to allocate new task user stack\r\n");
        return -1;
    }

    // Initialize user stack to zero
    memzero((unsigned long)current->user_stack, USER_STACK_SIZE);  

    // Mapping the user stack to the user virtual address
    unsigned long user_stack_pa = (unsigned long)VIRT_TO_PHYS(current->user_stack);
    if (mappages(current->pgd, USER_STACK_BASE, USER_STACK_SIZE, user_stack_pa) != 0) {
        dfree(current->user_program);
        muart_puts("Error: Failed to map user stack to virtual address space\r\n");
        return 0;
    }
```

* `move_to_user_mode` 會另外 map VA 0x3c000000 ~ 0x3fffffff in user mode to PA 0x3c000000 ~ 0x3fffffff (identity mapping)，因為在 lab website 提供的 `vm.img` 中會設定 framebuffer，這是一個 GPU peripheral，然後這個 user program 會直接 access 這塊 memory space 的，像這樣 : 
```
0x67c                  str    w0,  [x1]

x1 0x3c25_e76c
```
因為在 vm.img 是直接寫死的，`0x3c25_e76c` 會讓 MMU 認為是 user mode 的 VA，但因為我們並沒有建立這個 VA 跟 PA 的 mapping 關係 i.e, 沒有 create 相關的 page table entry， 為了讓 user program 可以直接 access 這塊 `0x3c000000 ~ 0x3fffffff` memory space，所以在這邊也做一個 identity mapping
```c
    // mapping the VA 0x3c000000 ~ 0x3fffffff in user mode to PA 0x3c000000 ~ 0x3fffffff (identity mapping)
    if (mappages(current->pgd, PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, PERIPHERAL_START) != 0) {
        muart_puts("Error: Failed to map peripheral memory to user virtual address space\r\n");
    }

```
最後 setup system registers 就可以跳到 user mode (EL0) :
* 把這個 thread 的 PGD 放到 TTBR0_EL1，
* 設定 SPSR 是 EL0，
* 設定 sp_el0 (EL0 用的 sp) 為 user stack 的 VA `USER_STACK_TOP 0x0000fffffffff000` 
* 最後將 0x0 放進 ELR_EL1，執行 eret 之後就會 switch 到 EL0，
 * 此時 0x0 就會被 MMU 用 TTBR0_EL1 translate 成 PA 去執行
 * 所以前面把 user program data map 到 0x0 跟 user stack map 到 USER_STACK_BASE 很重要，TTBR0_EL1 設對也很重要，這樣才能用到每一個 task 獨一無二的 mapping 關係


### Revisiting System Calls

#### fork()
Create child process with separate memory space:

1.  allocate 一塊 physical memory space 給 child task 放跟 parent 一樣的 user program 的 data，然後將這塊 physical memory space mapping 到 `USER_CODE_BASE 0x0`
   ```c
    // Allocate a physical memory space for the user program
    child->user_program_size = parent->user_program_size; 
    child->user_program = dmalloc(child->user_program_size);
    if (!child->user_program) {
        pid_free(child->pid);
        dfree(child->kernel_stack);
        dfree(child->user_stack);
        dfree(child);
        dfree(child->pgd);
        enable_irq_in_el1();
        return -1;
    } 

    // Copy the parent's user program data to the child's user program
    memcpy(child->user_program, parent->user_program, child->user_program_size);
    
    // Mapping the physical memory space of the user program to the user virtual address
    unsigned long user_program_pa = (unsigned long)VIRT_TO_PHYS(child->user_program);
    if (mappages(child->pgd, USER_CODE_BASE, child->user_program_size, user_program_pa) != 0) {
        dfree(child->user_program);
        muart_puts("Error: Failed to map user program to virtual address space\r\n");
        return 0;
    }
   ```
2.  allocate 一塊 physical memory space 給 child task 放 user mode 的 stack，stack 的內容跟 offset 都和 parent 一樣， 然後將這塊 physical memory space mapping 到 `USER_STACK_BASE 0x0000ffffffffb000`

    ```c
    memcpy(child->user_stack, parent->user_stack, USER_STACK_SIZE);
        
    // Mapping the user stack to the user virtual address
    unsigned long user_stack_pa = (unsigned long)VIRT_TO_PHYS(child->user_stack);
    if (mappages(child->pgd, USER_STACK_BASE, USER_STACK_SIZE, user_stack_pa) != 0) {
        dfree(child->user_program);
        muart_puts("Error: Failed to map user stack to virtual address space\r\n");
        return 0;
    }
    
    // Mapping the VA 0x3c000000 ~ 0x3fffffff in user mode to 0x3c000000 ~ 0x3fffffff (identity mapping)
    if (mappages(child->pgd, PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, PERIPHERAL_START) != 0) {
        muart_puts("Error: Failed to map peripheral memory to user virtual address space\r\n");
        return 0;
    }

    // Adjust child's SP_EL0 to point to child's user stack with same offset
    unsigned long parent_sp_offset = parent_tf->sp_el0 - USER_STACK_BASE;
    child_tf->sp_el0 = USER_STACK_BASE + parent_sp_offset;
    ```

* 當 context switch 到 child task 時，他會跳到 `ret_to_user`，然後用 trap frame 恢復在 user mode 的狀態
    * General purpose reg
    * sp_el0    (也要跟 parent 跳 fork 前的執行狀態一樣，所以才要算 offset 讓 sp_el0 指向正確的位址)
    * elr_el1   (跟 parent 一樣，跳回 fork 前執行的地方)
    * spsr_el1 = EL0

#### exec()  
`exec` syscall 會完全替換當前 process 的 memory，載入並執行新的 program。
```c
if (current->user_program) {
    dfree(current->user_program);  // clean up old program
    current->user_program = NULL;
    current->user_program_size = 0;
}

// Load new program
unsigned long new_program_addr = cpio_load_program(initramfs_addr, name);
```
阿在 cpio_load_program 中又會 
  1.  allocate 一塊 physical memory space 放 user program 的 data，然後將這塊 physical memory space mapping 到 `USER_CODE_BASE 0x0`
  2.  allocate 一塊 physical memory space 放 user mode 的 stack， 然後將這塊 physical memory space mapping 到 `USER_STACK_BASE 0x0000ffffffffb000`

然後他也要記得 map VA 0x3c000000 ~ 0x3fffffff 到 PA 0x3c000000 ~ 0x3fffffff

*  Update trap frame
Trap frame 保存了 user process 在 user mode 執行時的 context，因為 `exec` 會完全替換當前的 process，因此需要更新 trap frame，user process 原本的 trap frame 會像這樣
```c
struct trap_frame {
    unsigned long regs[31];     // 保留 user process switch 到 system call 前的 General purpose registers x0-x30
    unsigned long sp_el0;       // 保留 user process switch 到 system call 前的 Stack pointer
    unsigned long elr_el1;      // 保留 user process switch 到 system call 前的 Program counter
    unsigned long spsr_el1;     // 保留 user process switch 到 system call 前的 Processor state
};
```
在 system call 結束後 `retrun_from_syscall` 會把 trap frame 的這些 registers restore 回去，再 call `eret` 跳到 `elr_el1` 並把 Processor STATE 設成 `spsr_el1` (EL0t)。

* 因為我們的目標是 `exec` system call 結束後可以跳到新的 user program 去執行，所以這邊要更新 `elr_el1` 讓他指向新的 user program 的 start address，當然對每一個 user program 來說，他們的 entry point 都是 `0x0`。
* 更新 `sp_el0` 讓 thread 回到 user mode 後是用 user stack 的 stack pointer，這邊也是改成用 user stack 的 VA，(阿要記得做 map)
* 更新 `spsr_el1` 讓 thread 確實回到 user mode

```c
    // Update trap frame to start executing new program
    struct trap_frame* regs = task_pt_regs(current);
    
    // Reset user context
    memzero((unsigned long)regs, sizeof(*regs));
    
    // Set new program entry point
    regs->elr_el1 = USER_CODE_BASE;
    regs->sp_el0 = USER_STACK_TOP;  
    regs->spsr_el1 = 0;  // EL0t mode
```

#### mbox_call()
The following code is for mailbox initialize used in the labs.
```c
#define MBOX_REQUEST 0
#define MBOX_CH_PROP 8
#define MBOX_TAG_LAST 0

unsigned int __attribute__((aligned(16))) mbox[36];
unsigned int width, height, pitch, isrgb; /* dimensions and channel order */
unsigned char *lfb;                       /* raw frame buffer address */

mbox[0] = 35 * 4;
mbox[1] = MBOX_REQUEST;

mbox[2] = 0x48003; // set phy wh
mbox[3] = 8;
mbox[4] = 8;
mbox[5] = 1024; // FrameBufferInfo.width
mbox[6] = 768;  // FrameBufferInfo.height

mbox[7] = 0x48004; // set virt wh
mbox[8] = 8;
mbox[9] = 8;
mbox[10] = 1024; // FrameBufferInfo.virtual_width
mbox[11] = 768;  // FrameBufferInfo.virtual_height

mbox[12] = 0x48009; // set virt offset
mbox[13] = 8;
mbox[14] = 8;
mbox[15] = 0; // FrameBufferInfo.x_offset
mbox[16] = 0; // FrameBufferInfo.y.offset

mbox[17] = 0x48005; // set depth
mbox[18] = 4;
mbox[19] = 4;
mbox[20] = 32; // FrameBufferInfo.depth

mbox[21] = 0x48006; // set pixel order
mbox[22] = 4;
mbox[23] = 4;
mbox[24] = 1; // RGB, not BGR preferably

mbox[25] = 0x40001; // get framebuffer, gets alignment on request
mbox[26] = 8;
mbox[27] = 8;
mbox[28] = 4096; // FrameBufferInfo.pointer
mbox[29] = 0;    // FrameBufferInfo.size

mbox[30] = 0x40008; // get pitch
mbox[31] = 4;
mbox[32] = 4;
mbox[33] = 0; // FrameBufferInfo.pitch

mbox[34] = MBOX_TAG_LAST;

// this might not return exactly what we asked for, could be
// the closest supported resolution instead
if (mbox_call(MBOX_CH_PROP, mbox) && mbox[20] == 32 && mbox[28] != 0) {
  mbox[28] &= 0x3FFFFFFF; // convert GPU address to ARM address
  width = mbox[5];        // get actual physical width
  height = mbox[6];       // get actual physical height
  pitch = mbox[33];       // get number of bytes per line
  isrgb = mbox[24];       // get the actual channel order
  lfb = (void *)((unsigned long)mbox[28]);
} else {
  puts("Unable to set screen resolution to 1024x768x32\n");
}
```

因為 GPU 沒辦法看懂 VA，這邊要直接給 GPU PA，
需要在 kernel space 先 allocate 一塊 message buffer 作為 CPU 跟 GPU 溝通放東西的 buffer
```
使用者空間位址 (0xffffffffedd0)
    ↓ 
    ✗ GPU 無法存取（沒有使用者空間的頁表映射）

核心空間位址 (0xffff000000088000)
    ↓
    ✓ 可以轉換為實體位址 (0x00088000)
    ↓
    ✓ GPU 可以存取實體記憶體
```
```c
// Declare a message buffer for the communication between the CPU and GPU
volatile unsigned int __attribute__((aligned(16))) mailbox[64]; // 35 in the vm.img case

memcpy((unsigned int*)&mailbox, mbox, mailbox_size); // Copy the user-provided mailbox buffer to the kernel space mailbox buffer    
mailbox_call(ch, (volatile unsigned int*)VIRT_TO_PHYS(&mailbox));
memcpy(mbox, (unsigned int*)&mailbox, mailbox_size); // Copy the mailbox buffer back to the user-provided mailbox buffer
```
最後再將 kernel space 的 message buffer copy bck to user space 的 message buffer `mbox`

## Implementation Notes

### Page Table Entry Attributes

User space PTE configuration:
```c
#define USER_PTE_ATTR (PD_ACCESS | PD_USER_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_PAGE)
```

- `PD_ACCESS`: Access flag set to avoid access faults
- `PD_USER_ACCESS`: Allow user access (EL0)  
- `MAIR_IDX_NORMAL_NOCACHE`: Use normal memory without cache
- `PD_PAGE`: Mark as page entry (bits[1:0] = 11)

### Memory Barriers and TLB Management

Critical for correct virtual memory operation:
```c
dsb ish    // Data Synchronization Barrier - ensure memory operations complete
isb        // Instruction Synchronization Barrier - clear pipeline  
tlbi       // TLB Invalidation - clear stale translations
```

### Virtual to Physical Address Translation

Kernel uses identity mapping with offset:
```c
#define VIRT_TO_PHYS(addr) ((unsigned long)(addr) - 0xffff000000000000)
#define PHYS_TO_VIRT(addr) ((unsigned long)(addr) + 0xffff000000000000)
```
