#
# Copyright 2026, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

SHELL_COMPONENT_DIR := $(LIONSOS)/components/shell
SHELL_OBJ := \
	shell/shell0.o \
	shell/shell1.o \
	shell/shell2.o \
	shell/shell3.o

shell0.elf: shell/shell0.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

shell1.elf: shell/shell1.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

shell2.elf: shell/shell2.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

shell3.elf: shell/shell3.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

shell:
	mkdir -p shell

shell/shell0.o: $(SHELL_COMPONENT_DIR)/shell.c | shell $(LIONS_LIBC)/include
	$(CC) -c $(CFLAGS) -DSHELL_CORE_ID=0 $< -o $@

shell/shell1.o: $(SHELL_COMPONENT_DIR)/shell.c | shell $(LIONS_LIBC)/include
	$(CC) -c $(CFLAGS) -DSHELL_CORE_ID=1 $< -o $@

shell/shell2.o: $(SHELL_COMPONENT_DIR)/shell.c | shell $(LIONS_LIBC)/include
	$(CC) -c $(CFLAGS) -DSHELL_CORE_ID=2 $< -o $@

shell/shell3.o: $(SHELL_COMPONENT_DIR)/shell.c | shell $(LIONS_LIBC)/include
	$(CC) -c $(CFLAGS) -DSHELL_CORE_ID=3 $< -o $@

-include $(SHELL_OBJ:.o=.d)
