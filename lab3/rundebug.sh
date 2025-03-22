#!/bin/bash
gdb-multiarch -ex "file src/bootloader/bootloader.debug.elf" -ex "target remote :1234" -ex "b _start" 