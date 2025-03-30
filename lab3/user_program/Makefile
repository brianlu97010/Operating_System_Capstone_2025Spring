CROSS = aarch64-linux-gnu-
AS = $(CROSS)as
LD = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy

# 編譯組合語言文件
%.o: %.S
	$(AS) $< -o $@

# 生成 test1
test1: test1.o
	$(LD) -o test1.elf test1.o
	$(OBJCOPY) -O binary test1.elf test1

# 生成 test2
test2: test2.o
	$(LD) -o test2.elf test2.o
	$(OBJCOPY) -O binary test2.elf test2

# 更新 rootfs
update_rootfs: test1 test2
	@cp test1 ../rootfs/test1
	@cp test2 ../rootfs/test2
	@cd ../rootfs && find . | cpio -o -H newc > ../initramfs.cpio

# 清理
clean:
	rm -f *.o *.elf test1 test2
