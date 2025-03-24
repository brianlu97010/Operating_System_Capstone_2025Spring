# OSC 2025 | Lab 3 : Exception and Interrupts
## Basic Exercise 1 : Exceptions
```bash
make clean && make qemu
qemu-system-aarch64 -machine raspi3b -kernel kernel8.img -serial null -serial stdio -dtb bcm2710-rpi-3-b-plus.dtb -initrd initramfs.cpio
```
