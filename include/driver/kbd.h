#pragma once

#define I8042_DATA_REG   0x60  // Keyboard Data
#define I8042_STATUS_REG 0x64  // Keyboard Controller

#define I8042_OBUF_FULL 0x01
#define I8042_IBUF_FULL 0x02

// Special keycodes
#define NO 0

#define KEY_HOME 0xE0
#define KEY_END 0xE1
#define KEY_UP 0xE2
#define KEY_DN 0xE3
#define KEY_LF 0xE4
#define KEY_RT 0xE5
#define KEY_PGUP 0xE6
#define KEY_PGDN 0xE7
#define KEY_INS 0xE8
#define KEY_DEL 0xE9

#define KP_ENTER '\n'
#define KP_DIV   '/'