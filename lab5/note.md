## extract syscall.img from the initramfs and load it into memory:
`syscall.img` 從 `program_start_addr` 開始，到 `file_data_size` 結束

## 把 syscall.img 複製到 memory space 中
因為現在沒有 MMU 跟 Virtual Memory ，所以直接把 `syscall.img` 的 data 複製到指定的 physical memory space


在 cpio_exec 找到要執行的 user program 後，先把他 copy 到指定的 physical memory 位址，然後 create 一個 kernel thread 去