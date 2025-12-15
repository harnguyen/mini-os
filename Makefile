# MiniOS Makefile
# Build system for x86_64 educational operating system

# Cross-compiler toolchain (adjust if using different prefix)
AS = nasm
CC = x86_64-elf-gcc
LD = x86_64-elf-ld

# If cross-compiler not found, try native gcc with appropriate flags
ifeq ($(shell which $(CC) 2>/dev/null),)
    CC = gcc
    LD = ld
endif

# Directories
SRC_DIR = src
BUILD_DIR = build
ISO_DIR = $(BUILD_DIR)/isofiles

# Output files
KERNEL = $(BUILD_DIR)/kernel.bin
ISO = $(BUILD_DIR)/minios.iso

# Compiler flags
CFLAGS = -std=gnu11 \
         -ffreestanding \
         -fno-stack-protector \
         -fno-pic \
         -mno-red-zone \
         -mno-mmx \
         -mno-sse \
         -mno-sse2 \
         -mcmodel=kernel \
         -Wall \
         -Wextra \
         -O2 \
         -I include

# Linker flags
LDFLAGS = -T linker.ld \
          -nostdlib \
          -z max-page-size=0x1000

# Assembler flags
ASFLAGS = -f elf64

# Source files
ASM_SOURCES = $(SRC_DIR)/boot/boot.asm $(SRC_DIR)/boot/isr.asm
C_SOURCES = $(wildcard $(SRC_DIR)/kernel/*.c) \
            $(wildcard $(SRC_DIR)/boot/*.c) \
            $(wildcard $(SRC_DIR)/drivers/*.c) \
            $(wildcard $(SRC_DIR)/lib/*.c) \
            $(wildcard $(SRC_DIR)/net/*.c) \
            $(wildcard $(SRC_DIR)/shell/*.c)

# Object files
ASM_OBJECTS = $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
C_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS)

# Default target
.PHONY: all
all: $(ISO)

# Create kernel binary
$(KERNEL): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^

# Compile assembly files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

# Compile C files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# Create bootable ISO
$(ISO): $(KERNEL) grub.cfg
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.bin
	cp grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(ISO_DIR) 2>/dev/null

# Create disk image for ATA testing
$(BUILD_DIR)/disk.img:
	@mkdir -p $(BUILD_DIR)
	dd if=/dev/zero of=$@ bs=1M count=32
	@echo "MiniOS Test Disk" | dd of=$@ bs=1 seek=0 conv=notrunc

# Run in QEMU with ISO (requires grub-mkrescue)
.PHONY: run
run: $(ISO) $(BUILD_DIR)/disk.img
	qemu-system-x86_64 \
		-cdrom $(ISO) \
		-drive file=$(BUILD_DIR)/disk.img,format=raw,if=ide \
		-device virtio-net-pci,netdev=net0 \
		-netdev user,id=net0 \
		-m 128M \
		-serial stdio

# Run directly with QEMU multiboot (no GRUB needed)
.PHONY: run-direct
run-direct: $(KERNEL) $(BUILD_DIR)/disk.img
	qemu-system-x86_64 \
		-kernel $(KERNEL) \
		-drive file=$(BUILD_DIR)/disk.img,format=raw,if=ide \
		-device virtio-net-pci,netdev=net0 \
		-netdev user,id=net0 \
		-m 128M \
		-serial stdio

# Run with debug output
.PHONY: run-debug
run-debug: $(ISO) $(BUILD_DIR)/disk.img
	qemu-system-x86_64 \
		-cdrom $(ISO) \
		-drive file=$(BUILD_DIR)/disk.img,format=raw,if=ide \
		-device virtio-net-pci,netdev=net0 \
		-netdev user,id=net0 \
		-m 128M \
		-serial stdio \
		-d int,cpu_reset \
		-no-reboot

# Run with GDB debugging
.PHONY: run-gdb
run-gdb: $(ISO) $(BUILD_DIR)/disk.img
	qemu-system-x86_64 \
		-cdrom $(ISO) \
		-drive file=$(BUILD_DIR)/disk.img,format=raw,if=ide \
		-device virtio-net-pci,netdev=net0 \
		-netdev user,id=net0 \
		-m 128M \
		-serial stdio \
		-s -S

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# Create ISO using Docker (for macOS without grub-mkrescue)
.PHONY: iso-docker
iso-docker: $(KERNEL)
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.bin
	cp grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	@echo "Creating ISO with Docker (x86_64 container)..."
	docker run --rm --platform linux/amd64 -v "$(PWD)":/work -w /work ubuntu:22.04 bash -c \
		"apt-get update -qq && apt-get install -qq -y grub-pc-bin grub2-common xorriso mtools >/dev/null && \
		 grub-mkrescue -o build/minios.iso build/isofiles 2>&1"
	@echo "ISO created: build/minios.iso"

# Run after creating ISO with Docker
.PHONY: run-iso
run-iso: $(BUILD_DIR)/disk.img
	@test -f $(ISO) || (echo "Run 'make iso-docker' first to create the ISO"; exit 1)
	qemu-system-x86_64 \
		-cdrom $(ISO) \
		-drive file=$(BUILD_DIR)/disk.img,format=raw,if=ide \
		-device virtio-net-pci,netdev=net0 \
		-netdev user,id=net0 \
		-m 128M \
		-serial stdio

# Show help
.PHONY: help
help:
	@echo "MiniOS Build System"
	@echo "==================="
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build the kernel and create bootable ISO (default)"
	@echo "  run         - Run MiniOS in QEMU (requires grub-mkrescue)"
	@echo "  run-direct  - Run directly with QEMU multiboot (may not work with Multiboot2)"
	@echo "  run-debug   - Run with interrupt debugging"
	@echo "  run-gdb     - Run with GDB server (connect with 'target remote :1234')"
	@echo "  run-docker  - Run using Docker (works on any platform)"
	@echo "  clean       - Remove build artifacts"
	@echo "  help        - Show this help message"
	@echo ""
	@echo "Prerequisites:"
	@echo "  - x86_64-elf-gcc cross-compiler"
	@echo "  - NASM assembler"
	@echo "  - GRUB tools (grub-mkrescue) - or use run-docker"
	@echo "  - QEMU (qemu-system-x86_64)"
	@echo "  - xorriso"

