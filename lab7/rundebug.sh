#!/bin/bash
gdb-multiarch -ex "file src/kernel/kernel8.debug.elf" -ex "target remote :1234" -ex "b main" 