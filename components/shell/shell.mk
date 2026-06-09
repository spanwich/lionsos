#
# Copyright 2026, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

SHELL_COMPONENT_DIR := $(LIONSOS)/components/shell
SHELL_OBJ := shell/shell.o

shell.elf: $(SHELL_OBJ)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

shell:
	mkdir -p shell

shell/%.o: $(SHELL_COMPONENT_DIR)/%.c | shell $(LIONS_LIBC)/include
	$(CC) -c $(CFLAGS) $< -o $@

-include $(SHELL_OBJ:.o=.d)
