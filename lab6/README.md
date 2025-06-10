# OSC 2025 | Lab 6: Virtual Memory
> [!IMPORTANT]
> Todo :
> Advanced Exercise 1 - 

## Basic Exercise 1 - Virtual Memory in Kernel Space
Three level translation
Each page table has 512 entries
* First 1GB memory space  (RAM and GPU peripherals)
  * 0x00000000 ~ 0x3c000000 : Normal memory without cache 
  * 0x3c000000 ~ 0x3fffffff : Device memory nGnRnE
* Second 1GB memory space 
  * 0x40000000 ~ 0x7fffffff : Device memory nGnRnE

## Basic Exercise 2 - Virtual Memory in User Space 
### PGD Allocation
To isolate user processes, you should create an address space for each of them. Hence, the kernel should allocate one PGD for each process when it creates a process.

#### Data Structure
* `struct cpu_text` 新增一個 `phy_addr_pgd` 用來放 PGD 的 PA，為了在 context switch 後 load 到 TTBR0_EL1
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
這個 offset 很重要，對應到 `struct cpu_text` 裡的相對位置

* `struct task_struct` 新增一個 `unsigned long* pgd` 用來用 PGD 的 VA，將 PA mapping 到 user mode 的 VA 時會用到  (作為 `mappages()` 的參數)


### Map to User mode
call the `mappages` in `vm.c` : 
4 level 的 translation，從 VA 取出各個 page table 用到的 index
```c
switch (level) {
    case 0: idx = PGD_IDX(va); break;  // PGD level
    case 1: idx = PUD_IDX(va); break;  // PUD level  
    case 2: idx = PMD_IDX(va); break;  // PMD level
    default: return NULL;
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
PTE attribute configuration : 
Refer to hackmd note
Bits[10] = 1
Bits[6] = 1
Bits[3:2] = 01
Bits[1:0] = 11


### 如何使用 user mode 的 VA
> 先釐清 : 
> 只要是 VA = 0x0000_0000_0000_0000 ~ 0x0000_FFFF_FFFF_FFFF ，ARM v8-A 會自動將它視為在 user mode access 的 memory address，會使用 TTBR0_EL1 作為 PGD (TTBR0_EL1 是 context switch 後每個 task 各自有的 page table address，這樣才能讓每個 user process 從它們的角度來看都是有完整 0x0 ~ 0xfffffffffffff 的 memory space)
> 反之，只要是 VA = 0xFFFF_0000_0000_0000 ~ 0xFFFF_FFFF_FFFF_FFFF，ARM v8-A 會自動將它視為在 kernel mode access 的 memory address，會使用 TTBR1_EL1 作為 PGD (這邊是在 boot.S 就 defined 好了)

#### 讓 kernel thread switch 到 user mode，並執行 cpio archive initramfs.cpio 中的 user program :

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
    muart_puts("Mapped user program (physical address) : ");
    muart_send_hex(user_program_pa);
    muart_puts(" to user virtual address 0x0\r\n");
    

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


* `move_to_user_mode` 會另外 map VA 0x3c000000 ~ 0x3fffffff in user mode to PA 0x3c000000 ~ 0x3fffffff (identity mapping)，因為在 lab website 提供的 `vm.img` 中會設定 framebuffer，這是一個 GPU periphral，然後這個 user program 是直接 access 這塊 memory space 的，像這樣 : 
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
把這個 thread 的 PGD 放到 TTBR0_EL1，
設定 SPSR 是 EL0，
設定 sp_el0 (EL0 用的 sp) 為 user stack 的 VA `USER_STACK_TOP 0x0000fffffffff000` 
最後將 0x0 放進 ELR_EL1，執行 eret 之後就會 switch 到 EL0，
此時 0x0 就會被 MMU 用 TTBR0_EL1 translate 成 PA 去執行
所以前面把 user program data map 到 0x0 跟 user stack map 到 USER_STACK_BASE 很重要，TTBR0_EL1 設對也很重要，這樣才能用到每一個 task 獨一無二的 mapping 關係



### Revisit system call
#### `exec` 
`exec` syscall 會完全替換當前 process 的 memory，載入並執行新的 program。當調用 `exec` 時：

* PID 保持不變
* 完全替換當前 process 的 memory space 
* 重置 process 的各種屬性和狀態

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

#### `fork`
直接
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
    因為當 context switch 到 child task 時，他會跳到 ret_to_user，然後用 trap frame 回復在 user mode 的狀態
    * General purpose reg
    * sp_el0    (也要跟 parent 跳 fork 前的執行狀態一樣，所以才要算 offset 讓 sp_el0 指向正確的位址)
    * elr_el1   (跟 parent 一樣，跳回 fork 前執行的地方)
    * spsr_el1 

#### `mailbox`
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
