#include "pid.h"
#include "bitmap.h"
#include "muart.h"

/* Global PID bitmap */
static pid_bitmap_t pid_bitmap;

/**
 * 找到位圖中第一個為0的位元，從offset開始查找
 * 
 * @param bitmap  位圖的基地址
 * @param size    位圖的總位元數
 * @param offset  開始搜索的位置
 * @return        找到的第一個為0的位元索引，若未找到則返回size
 */
unsigned int find_next_zero_bit(const unsigned long *bitmap, unsigned int size, unsigned int offset) {
    // 如果偏移已經超出範圍，直接返回size
    if (offset >= size) {
        return size;
    }
    
    // 計算起始元素和位偏移
    unsigned int word_offset = BIT_WORD(offset);
    unsigned int bit_offset = offset % BITS_PER_LONG;
    unsigned long word;
    
    // 檢查當前元素中偏移之後的位
    word = bitmap[word_offset] >> bit_offset;
    
    // 如果當前字中在偏移之後有0位（也就是取反後有1位）
    if (~word != 0) {
        bit_offset += __builtin_ffsl(~word) - 1;
        if (bit_offset < BITS_PER_LONG && offset + bit_offset - (offset % BITS_PER_LONG) < size) {
            return offset + bit_offset - (offset % BITS_PER_LONG);
        }
    }
    
    // 檢查後續元素
    size -= (offset + (BITS_PER_LONG - bit_offset));
    word_offset++;
    
    // 處理整個unsigned long的情況
    while (size >= BITS_PER_LONG) {
        if (bitmap[word_offset] != ~0UL) {
            // 找到第一個為0的位
            bit_offset = __builtin_ffsl(~bitmap[word_offset]) - 1;
            return word_offset * BITS_PER_LONG + bit_offset;
        }
        
        word_offset++;
        size -= BITS_PER_LONG;
    }
    
    // 處理剩餘的不足一個unsigned long的位
    if (size > 0) {
        word = bitmap[word_offset];
        if (word != ~0UL) {
            bit_offset = __builtin_ffsl(~word) - 1;
            if (bit_offset < size) {
                return word_offset * BITS_PER_LONG + bit_offset;
            }
        }
    }
    
    // 如果在往後找不到，從頭開始找（環形搜索）
    word_offset = BIT_WORD(MIN_PID);
    unsigned int search_limit = BIT_WORD(offset);
    
    for (; word_offset < search_limit; word_offset++) {
        if (bitmap[word_offset] != ~0UL) {
            bit_offset = __builtin_ffsl(~bitmap[word_offset]) - 1;
            return word_offset * BITS_PER_LONG + bit_offset;
        }
    }
    
    // 未找到為0的位
    return size;
}

/**
 * 找到位圖中第一個為0的位元
 * 
 * @param bitmap  位圖的基地址
 * @param size    位圖的總位元數
 * @return        找到的第一個為0的位元索引，若未找到則返回size
 */
unsigned int find_first_zero_bit(const unsigned long *bitmap, unsigned int size) {
    return find_next_zero_bit(bitmap, size, MIN_PID);
}

/**
 * 初始化 PID 位元圖
 */
void pid_bitmap_init(void) {
    // 清空整個位元圖
    for (int i = 0; i < BITS_TO_LONGS(MAX_PID + 1); i++) {
        pid_bitmap.pid_bitmap[i] = 0;
    }
    
    // 保留PID 0和1
    set_bit(0, pid_bitmap.pid_bitmap); // 保留給idle task
    set_bit(1, pid_bitmap.pid_bitmap); // 保留給init task
    
    // 設置last_pid
    pid_bitmap.last_pid = 1;
    
    muart_puts("PID bitmap initialized, PIDs range from ");
    muart_send_dec(MIN_PID);
    muart_puts(" to ");
    muart_send_dec(MAX_PID);
    muart_puts("\r\n");
}

/**
 * 分配一個新的PID
 * 
 * @return        分配的PID，若失敗則返回-1
 */
pid_t pid_alloc(void) {
    // 從上次分配的PID之後開始搜索
    int offset = pid_bitmap.last_pid + 1;
    if (offset > MAX_PID) {
        offset = MIN_PID;  // 回環到最小可用PID值
    }
    
    // 查找下一個可用PID
    int pid = find_next_zero_bit(pid_bitmap.pid_bitmap, MAX_PID + 1, offset);
    
    // 檢查找到的PID是否有效
    if (pid < MIN_PID || pid > MAX_PID) {
        // 如果沒找到或無效，從最小PID開始重新查找
        pid = find_next_zero_bit(pid_bitmap.pid_bitmap, MAX_PID + 1, MIN_PID);
        
        // 再次檢查
        if (pid < MIN_PID || pid > MAX_PID) {
            muart_puts("Error: No free PID available\r\n");
            return -1;
        }
    }
    
    // 標記PID為已分配
    set_bit(pid, pid_bitmap.pid_bitmap);
    pid_bitmap.last_pid = pid;
    
    return pid;
}

/**
 * 釋放一個已分配的PID
 * 
 * @param pid     要釋放的PID
 */
void pid_free(pid_t pid) {
    // 檢查PID是否有效且已分配
    if (pid < MIN_PID || pid > MAX_PID) {
        muart_puts("Error: Attempted to free invalid PID ");
        muart_send_dec(pid);
        muart_puts(" (out of range)\r\n");
        return;
    }
    
    if (!test_bit(pid, pid_bitmap.pid_bitmap)) {
        muart_puts("Warning: Attempting to free unallocated PID ");
        muart_send_dec(pid);
        muart_puts("\r\n");
        return;
    }
    
    // 釋放PID
    clear_bit(pid, pid_bitmap.pid_bitmap);
} 