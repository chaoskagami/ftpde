#pragma once

#ifdef _3DS
#include <3ds.h>
void console_init(void);

__attribute__((format(printf, 1, 2))) void console_print(const char *fmt, ...);
void console_render(void);

#else
#define console_init(args) {}
#define console_print printf
#define console_render(args) {}
#endif

__attribute__((format(printf, 1, 2))) void console_set_status(const char *fmt, ...);

// These are ansi sequences. They work on linux. Drunkard.
#define ESC(x) "\x1b[" #x
#define RESET ESC(0m)
#define BLACK ESC(30m)
#define RED ESC(31;1m)
#define GREEN ESC(32;1m)
#define YELLOW ESC(33;1m)
#define BLUE ESC(34;1m)
#define MAGENTA ESC(35;1m)
#define CYAN ESC(36;1m)
#define WHITE ESC(37;1m)
