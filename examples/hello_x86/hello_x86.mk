#
# Copyright 2026, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

TOOLCHAIN ?= clang
CC := $(TOOLCHAIN)
LD := $(TOOLCHAIN)
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit
BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
SYSTEM_FILE := hello_x86.system
IMAGE_FILE := hello_x86.img
REPORT_FILE := report.txt
KERNEL_ELF := $(BOARD_DIR)/elf/sel4_32.elf

all: ${IMAGE_FILE}

CFLAGS += \
	-target x86_64-none-elf \
	-ffreestanding \
	-fno-stack-protector \
	-fno-pic \
	-mno-red-zone \
	-Wall \
	-Werror \
	-O2 \
	-g \
	-I$(BOARD_DIR)/include

LDFLAGS += \
	-target x86_64-none-elf \
	-nostdlib \
	-L$(BOARD_DIR)/lib \
	-Wl,-T,$(BOARD_DIR)/lib/microkit.ld

LIBS := -lmicrokit

hello.o: $(HELLO_X86_DIR)/hello.c
	$(CC) $(CFLAGS) -c -o $@ $<

hello.elf: hello.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

$(SYSTEM_FILE): $(HELLO_X86_DIR)/hello_x86.system
	cp $< $@

$(IMAGE_FILE) $(REPORT_FILE): hello.elf $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

qemu: $(IMAGE_FILE)
	qemu-system-x86_64 \
		-enable-kvm \
		-cpu host \
		-machine q35,accel=kvm \
		-m 1G \
		-display none \
		-serial mon:stdio \
		-kernel $(KERNEL_ELF) \
		-initrd $(IMAGE_FILE)

clean:
	rm -f hello.o hello.elf $(SYSTEM_FILE) $(IMAGE_FILE) $(REPORT_FILE)

clobber: clean

FORCE:
