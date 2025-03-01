make debug
echo "Waiting for gdb connecting..."
qemu-system-aarch64 -m 1024 -M raspi3b -serial null -serial stdio -display none  -kernel kernel8.debug.img -s -S
