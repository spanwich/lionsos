/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>

void init(void)
{
    microkit_dbg_puts("HELLO_X86|INFO: LionsOS hello-world started on x86_64_generic_vtx\n");
}

void notified(microkit_channel ch)
{
    (void)ch;
}

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo)
{
    (void)ch;
    return msginfo;
}
