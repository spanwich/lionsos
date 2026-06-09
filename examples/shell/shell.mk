#
# Copyright 2026, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

IMAGES := \
	serial_driver.elf \
	serial_virt_rx.elf \
	serial_virt_tx.elf \
	shell0.elf \
	shell1.elf \
	shell2.elf \
	shell3.elf

SUPPORTED_BOARDS := \
	qemu_virt_aarch64

TOOLCHAIN ?= clang
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit
SDDF := $(LIONSOS)/dep/sddf
LIBMICROKITCO_PATH := $(LIONSOS)/dep/libmicrokitco
SYSTEM_FILE := shell.system
IMAGE_FILE := shell.img
REPORT_FILE := report.txt

all: $(IMAGE_FILE)

include $(SDDF)/tools/make/board/common.mk

METAPROGRAM := $(SHELL_SRC_DIR)/meta.py

CFLAGS += \
	-I$(LIONSOS)/include \
	-I$(SDDF)/include \
	-I$(SDDF)/include/microkit \
	-I$(LIBMICROKITCO_PATH)

include $(LIONSOS)/lib/libc/libc.mk

LDFLAGS := -L$(BOARD_DIR)/lib -L$(LIONS_LIBC)/lib
LIBS := -lmicrokit -Tmicrokit.ld -lc libsddf_util_debug.a

SDDF_LIBC_INCLUDE := $(LIONS_LIBC)/include
include $(SDDF)/util/util.mk
include $(SDDF)/drivers/serial/$(UART_DRIV_DIR)/serial_driver.mk
include $(SDDF)/serial/components/serial_components.mk
include $(LIONSOS)/components/shell/shell.mk

$(IMAGES): $(LIONS_LIBC)/lib/libc.a libsddf_util_debug.a

$(SYSTEM_FILE): $(METAPROGRAM) $(IMAGES) $(DTB)
	PYTHONPATH=$(SDDF)/tools/meta:$$PYTHONPATH $(PYTHON) $(METAPROGRAM) --sddf $(SDDF) --board $(MICROKIT_BOARD) --dtb $(DTB) --output . --sdf $(SYSTEM_FILE)
	$(OBJCOPY) --update-section .device_resources=serial_driver_device_resources.data serial_driver.elf
	$(OBJCOPY) --update-section .serial_driver_config=serial_driver_config.data serial_driver.elf
	$(OBJCOPY) --update-section .serial_virt_tx_config=serial_virt_tx.data serial_virt_tx.elf
	$(OBJCOPY) --update-section .serial_virt_rx_config=serial_virt_rx.data serial_virt_rx.elf
	$(OBJCOPY) --update-section .serial_client_config=serial_client_shell0.data shell0.elf
	$(OBJCOPY) --update-section .serial_client_config=serial_client_shell1.data shell1.elf
	$(OBJCOPY) --update-section .serial_client_config=serial_client_shell2.data shell2.elf
	$(OBJCOPY) --update-section .serial_client_config=serial_client_shell3.data shell3.elf
	touch $@

$(IMAGE_FILE) $(REPORT_FILE): $(IMAGES) $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

qemu: $(IMAGE_FILE)
	$(QEMU) -machine virt,virtualization=on \
		-cpu cortex-a53 \
		$(if $(findstring smp,$(MICROKIT_CONFIG)),-smp 4,) \
		-serial mon:stdio \
		-device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 \
		-m size=2G \
		-nographic \
		-global virtio-mmio.force-legacy=false

clean::
	rm -f $(IMAGES) $(SYSTEM_FILE) $(IMAGE_FILE) $(REPORT_FILE)
	rm -rf shell

clobber:: clean

FORCE:

$(SDDF)/tools/make/board/common.mk $(SDDF)/drivers/serial/$(UART_DRIV_DIR)/serial_driver.mk $(SDDF)/serial/components/serial_components.mk $(SDDF)/include &:
	cd $(LIONSOS); git submodule update --init dep/sddf
