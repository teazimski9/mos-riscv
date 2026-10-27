TARGET := mos
CC := clang

CFLAGS := --target=riscv64-unknown-elf -march=rv64imafdc -mabi=lp64d -mcmodel=medany \
          -ffreestanding -fno-builtin -fno-stack-protector \
          -Wall -Wextra -Werror -O2
ASFLAGS := --target=riscv64-unknown-elf -march=rv64imafdc -mabi=lp64d
LDFLAGS := --target=riscv64-unknown-elf -march=rv64imafdc -mabi=lp64d \
           -fuse-ld=lld -nostdlib -Wl,-T,link.ld -Wl,--build-id=none

OBJECTS := entry.o main.o uart.o math/math_core.o math/linalg.o math/rpn_stack.o \
           test/forth/forth.o

.PHONY: all run clean

all: $(TARGET).elf

$(TARGET).elf: $(OBJECTS) link.ld

	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(CC) $(ASFLAGS) -c -o $@ $<

run: $(TARGET).elf

	qemu-system-riscv64 -machine virt -nographic -bios none -kernel $(TARGET).elf

clean:

	rm -f $(OBJECTS) $(TARGET).elf
