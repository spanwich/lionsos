#
# Copyright 2026, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

ifeq ($(strip $(MICROKIT_SDK)),)
$(error MICROKIT_SDK must be specified)
endif
override MICROKIT_SDK := $(abspath ${MICROKIT_SDK})

export MICROKIT_CONFIG ?= debug
export BUILD_DIR ?= $(abspath build)
export MICROKIT_BOARD ?= qemu_virt_aarch64
export SHELL_SRC_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
export LIONSOS := $(abspath ../..)

IMAGE_FILE := $(BUILD_DIR)/shell.img
REPORT_FILE := $(BUILD_DIR)/report.txt

all: ${IMAGE_FILE}

qemu ${IMAGE_FILE} ${REPORT_FILE} clean clobber: ${BUILD_DIR}/Makefile FORCE
	${MAKE} -C ${BUILD_DIR} MICROKIT_SDK=${MICROKIT_SDK} $(notdir $@)

${BUILD_DIR}/Makefile: shell.mk Makefile
	mkdir -p ${BUILD_DIR}
	echo "MICROKIT_SDK ?= ${MICROKIT_SDK}" > $@
	echo "MICROKIT_BOARD ?= ${MICROKIT_BOARD}" >> $@
	echo "MICROKIT_CONFIG ?= ${MICROKIT_CONFIG}" >> $@
	echo "BUILD_DIR := ${BUILD_DIR}" >> $@
	echo "SHELL_SRC_DIR ?= ${SHELL_SRC_DIR}" >> $@
	echo "LIONSOS := ${LIONSOS}" >> $@
	cat shell.mk >> $@

FORCE:
