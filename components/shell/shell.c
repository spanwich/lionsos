/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <microkit.h>
#include <sddf/serial/config.h>
#include <sddf/serial/queue.h>

#define SHELL_LINE_MAX 128
#define SHELL_PROMPT "lions> "
#define SHELL_NUM_CORES 4
#define SHELL_INPUT_CH_BASE 20
#define SHELL_ACTIVATE_CH_BASE 30
#define SHELL_REMOTE_INPUT_CHAR 1
#define SHELL_NO_HANDOFF UINT32_MAX

#ifndef SHELL_CORE_ID
#define SHELL_CORE_ID 0
#endif

__attribute__((__section__(".serial_client_config"))) serial_client_config_t serial_config;

static serial_queue_handle_t serial_rx_queue_handle;
static serial_queue_handle_t serial_tx_queue_handle;
static char line[SHELL_LINE_MAX];
static uint16_t line_len;
static uint32_t command_count;
static bool active;

#if SHELL_CORE_ID == 0
static uint8_t active_shell;
#endif

static void shell_write(const char *s)
{
    size_t len = strlen(s);
    while (len > 0) {
        uint32_t written = serial_enqueue_batch(&serial_tx_queue_handle, len, s);
        if (written == 0) {
            break;
        }

        s += written;
        len -= written;
        microkit_notify(serial_config.tx.id);
    }
}

static void shell_write_u32(uint32_t value)
{
    char buf[11];
    uint8_t idx = sizeof(buf);

    buf[--idx] = '\0';
    do {
        buf[--idx] = (char)('0' + (value % 10));
        value /= 10;
    } while (value != 0);

    shell_write(&buf[idx]);
}

static char *skip_spaces(char *s)
{
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    return s;
}

static bool parse_core(char *s, uint8_t *core)
{
    s = skip_spaces(s);
    if (s[0] < '0' || s[0] >= (char)('0' + SHELL_NUM_CORES)) {
        return false;
    }
    if (skip_spaces(s + 1)[0] != '\0') {
        return false;
    }

    *core = (uint8_t)(s[0] - '0');
    return true;
}

static void print_identity(void)
{
    shell_write("shell-pd: shell");
    shell_write_u32(SHELL_CORE_ID);
    shell_write("\r\n");
    shell_write("core: ");
    shell_write_u32(SHELL_CORE_ID);
    shell_write("\r\n");
    shell_write("active: ");
    shell_write(active ? "yes\r\n" : "no\r\n");
}

static uint32_t request_handoff(uint8_t core)
{
    if (core == SHELL_CORE_ID) {
        return SHELL_NO_HANDOFF;
    }

    active = false;
    shell_write("handoff to shell");
    shell_write_u32(core);
    shell_write("\r\n");
    return core;
}

static uint32_t run_command(char *cmd)
{
    cmd = skip_spaces(cmd);
    command_count++;

    if (cmd[0] == '\0') {
        return SHELL_NO_HANDOFF;
    }

    if (!strcmp(cmd, "help")) {
        shell_write("commands:\r\n");
        shell_write("  help       show commands\r\n");
        shell_write("  core N     hand off to shell N, N=0..3\r\n");
        shell_write("  hello      print a greeting\r\n");
        shell_write("  echo TEXT  print TEXT\r\n");
        shell_write("  status     show active shell and core\r\n");
        shell_write("  version    show component version\r\n");
        shell_write("  clear      clear the terminal\r\n");
        return SHELL_NO_HANDOFF;
    }

    if (!strncmp(cmd, "core", 4) && (cmd[4] == '\0' || cmd[4] == ' ' || cmd[4] == '\t')) {
        uint8_t core;
        if (!parse_core(cmd + 4, &core)) {
            shell_write("usage: core N, where N is 0..3\r\n");
            return SHELL_NO_HANDOFF;
        }
        return request_handoff(core);
    }

    if (!strcmp(cmd, "hello")) {
        shell_write("hello from shell");
        shell_write_u32(SHELL_CORE_ID);
        shell_write("\r\n");
        return SHELL_NO_HANDOFF;
    }

    if (!strncmp(cmd, "echo", 4) && (cmd[4] == '\0' || cmd[4] == ' ' || cmd[4] == '\t')) {
        shell_write(skip_spaces(cmd + 4));
        shell_write("\r\n");
        return SHELL_NO_HANDOFF;
    }

    if (!strcmp(cmd, "status")) {
        shell_write("shell: running\r\n");
        print_identity();
        shell_write("serial-rx: enabled\r\n");
        shell_write("commands: ");
        shell_write_u32(command_count);
        shell_write("\r\n");
        return SHELL_NO_HANDOFF;
    }

    if (!strcmp(cmd, "version")) {
        shell_write("Shell-PD v0.1\r\n");
        return SHELL_NO_HANDOFF;
    }

    if (!strcmp(cmd, "clear")) {
        shell_write("\x1b[2J\x1b[H");
        return SHELL_NO_HANDOFF;
    }

    shell_write("unknown command: ");
    shell_write(cmd);
    shell_write("\r\n");
    return SHELL_NO_HANDOFF;
}

static void prompt(void)
{
    if (active) {
        shell_write(SHELL_PROMPT);
    }
}

static uint32_t accept_char(char c)
{
    if (c == '\r' || c == '\n') {
        shell_write("\r\n");
        line[line_len] = '\0';
        uint32_t handoff = run_command(line);
        line_len = 0;
        prompt();
        return handoff;
    }

    if (c == '\b' || c == 0x7f) {
        if (line_len > 0) {
            line_len--;
            shell_write("\b \b");
        }
        return SHELL_NO_HANDOFF;
    }

    if (c < 0x20 || c > 0x7e) {
        return SHELL_NO_HANDOFF;
    }

    if (line_len + 1 >= SHELL_LINE_MAX) {
        shell_write("\r\nline too long\r\n");
        line_len = 0;
        prompt();
        return SHELL_NO_HANDOFF;
    }

    line[line_len++] = c;
    char echo[2] = { c, '\0' };
    shell_write(echo);
    return SHELL_NO_HANDOFF;
}

#if SHELL_CORE_ID == 0
static void activate_shell(uint8_t shell)
{
    active_shell = shell;
    if (shell == 0) {
        active = true;
        shell_write("activated shell0 on core 0\r\n");
        prompt();
        return;
    }

    microkit_notify(SHELL_ACTIVATE_CH_BASE + shell);
}

static void forward_char(uint8_t shell, char c)
{
    microkit_mr_set(0, (uint32_t)c);
    microkit_msginfo msginfo = microkit_ppcall(SHELL_INPUT_CH_BASE + shell,
                                               microkit_msginfo_new(SHELL_REMOTE_INPUT_CHAR, 1));
    if (microkit_msginfo_get_count(msginfo) < 1) {
        return;
    }

    uint32_t handoff = microkit_mr_get(0);
    if (handoff < SHELL_NUM_CORES) {
        activate_shell((uint8_t)handoff);
    }
}
#endif

static void announce_ready(void)
{
    shell_write("\r\nShell-PD ready: shell");
    shell_write_u32(SHELL_CORE_ID);
    shell_write(" on core ");
    shell_write_u32(SHELL_CORE_ID);
    shell_write("\r\n");
}

void init(void)
{
    assert(serial_config_check_magic(&serial_config));

    if (serial_config.rx.queue.vaddr != NULL) {
        serial_queue_init(&serial_rx_queue_handle, serial_config.rx.queue.vaddr, serial_config.rx.data.size,
                          serial_config.rx.data.vaddr);
    }
    serial_queue_init(&serial_tx_queue_handle, serial_config.tx.queue.vaddr, serial_config.tx.data.size,
                      serial_config.tx.data.vaddr);

#if SHELL_CORE_ID == 0
    active = true;
    active_shell = 0;
    announce_ready();
    shell_write("Type 'help'.\r\n");
    prompt();
#else
    active = false;
#endif
}

void notified(microkit_channel ch)
{
#if SHELL_CORE_ID != 0
    if (ch == SHELL_ACTIVATE_CH_BASE + SHELL_CORE_ID) {
        active = true;
        announce_ready();
        prompt();
        return;
    }
#endif

#if SHELL_CORE_ID == 0
    if (ch != serial_config.rx.id) {
        return;
    }

    char c;
    while (!serial_queue_empty(&serial_rx_queue_handle, serial_rx_queue_handle.queue->head)) {
        int err = serial_dequeue(&serial_rx_queue_handle, &c);
        assert(!err);
        if (active_shell == 0) {
            uint32_t handoff = accept_char(c);
            if (handoff < SHELL_NUM_CORES) {
                activate_shell((uint8_t)handoff);
            }
        } else {
            forward_char(active_shell, c);
        }
    }
#else
    (void)ch;
#endif
}

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo)
{
    (void)ch;
    if (microkit_msginfo_get_label(msginfo) != SHELL_REMOTE_INPUT_CHAR) {
        microkit_mr_set(0, SHELL_NO_HANDOFF);
        return microkit_msginfo_new(0, 1);
    }

    uint32_t handoff = accept_char((char)microkit_mr_get(0));
    microkit_mr_set(0, handoff);
    return microkit_msginfo_new(0, 1);
}
