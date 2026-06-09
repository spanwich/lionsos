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

__attribute__((__section__(".serial_client_config"))) serial_client_config_t serial_config;

static serial_queue_handle_t serial_rx_queue_handle;
static serial_queue_handle_t serial_tx_queue_handle;
static char line[SHELL_LINE_MAX];
static uint16_t line_len;
static uint32_t command_count;

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

static void run_command(char *cmd)
{
    cmd = skip_spaces(cmd);
    command_count++;

    if (cmd[0] == '\0') {
        return;
    }

    if (!strcmp(cmd, "help")) {
        shell_write("commands:\r\n");
        shell_write("  help       show commands\r\n");
        shell_write("  hello      print a greeting\r\n");
        shell_write("  echo TEXT  print TEXT\r\n");
        shell_write("  status     show shell state\r\n");
        shell_write("  version    show component version\r\n");
        shell_write("  clear      clear the terminal\r\n");
        return;
    }

    if (!strcmp(cmd, "hello")) {
        shell_write("hello from the LionsOS Shell-PD\r\n");
        return;
    }

    if (!strncmp(cmd, "echo", 4) && (cmd[4] == '\0' || cmd[4] == ' ' || cmd[4] == '\t')) {
        shell_write(skip_spaces(cmd + 4));
        shell_write("\r\n");
        return;
    }

    if (!strcmp(cmd, "status")) {
        shell_write("shell: running\r\n");
        shell_write("serial-rx: enabled\r\n");
        shell_write("commands: ");
        shell_write_u32(command_count);
        shell_write("\r\n");
        return;
    }

    if (!strcmp(cmd, "version")) {
        shell_write("Shell-PD v0.1\r\n");
        return;
    }

    if (!strcmp(cmd, "clear")) {
        shell_write("\x1b[2J\x1b[H");
        return;
    }

    shell_write("unknown command: ");
    shell_write(cmd);
    shell_write("\r\n");
}

static void prompt(void)
{
    shell_write(SHELL_PROMPT);
}

static void accept_char(char c)
{
    if (c == '\r' || c == '\n') {
        shell_write("\r\n");
        line[line_len] = '\0';
        run_command(line);
        line_len = 0;
        prompt();
        return;
    }

    if (c == '\b' || c == 0x7f) {
        if (line_len > 0) {
            line_len--;
            shell_write("\b \b");
        }
        return;
    }

    if (c < 0x20 || c > 0x7e) {
        return;
    }

    if (line_len + 1 >= SHELL_LINE_MAX) {
        shell_write("\r\nline too long\r\n");
        line_len = 0;
        prompt();
        return;
    }

    line[line_len++] = c;
    char echo[2] = { c, '\0' };
    shell_write(echo);
}

void init(void)
{
    assert(serial_config_check_magic(&serial_config));
    assert(serial_config.rx.queue.vaddr != NULL);

    serial_queue_init(&serial_rx_queue_handle, serial_config.rx.queue.vaddr, serial_config.rx.data.size,
                      serial_config.rx.data.vaddr);
    serial_queue_init(&serial_tx_queue_handle, serial_config.tx.queue.vaddr, serial_config.tx.data.size,
                      serial_config.tx.data.vaddr);

    shell_write("\r\nShell-PD ready. Type 'help'.\r\n");
    prompt();
}

void notified(microkit_channel ch)
{
    if (ch != serial_config.rx.id) {
        return;
    }

    char c;
    while (!serial_queue_empty(&serial_rx_queue_handle, serial_rx_queue_handle.queue->head)) {
        int err = serial_dequeue(&serial_rx_queue_handle, &c);
        assert(!err);
        accept_char(c);
    }
}

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo)
{
    (void)ch;
    return msginfo;
}
