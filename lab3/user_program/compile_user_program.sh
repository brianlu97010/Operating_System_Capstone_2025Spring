aarch64-linux-gnu-as -o test1.o test1.S
aarch64-linux-gnu-ld -o test1.elf test1.o
aarch64-linux-gnu-objcopy -O binary test1.elf test1
cd ..
cp user_program/test1 rootfs/test1 
cd rootfs 
find . | cpio -o -H newc > ../initramfs.cpio 
cd ..