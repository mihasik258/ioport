#include "command_processor.h"
#include "io_access.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

static int parse_number(const char* str, uintptr_t* value) {}

static void handle_read_command(const char* cmd, const char* arg, 
                               uint8_t (*read_byte)(uintptr_t),
                               uint16_t (*read_word)(uintptr_t),
                               uint32_t (*read_dword)(uintptr_t)) {}

static void handle_write_command(const char* cmd, const char* arg1, const char* arg2,
                                void (*write_byte)(uintptr_t, uint8_t),
                                void (*write_word)(uintptr_t, uint16_t),
                                void (*write_dword)(uintptr_t, uint32_t)) {}

int process_command(const char* line) {}

void print_help(void) {
    printf("Available commands:\n"
           "  iorb <addr>     - Read byte from IO address\n"
           "  iorw <addr>     - Read word from IO address\n"
           "  iord <addr>     - Read double word from IO address\n"
           "  iowb <addr> <data> - Write byte to IO address\n"
           "  ioww <addr> <data> - Write word to IO address\n"
           "  iowd <addr> <data> - Write double word to IO address\n"
           "  help            - Show this help message\n"
           "  quit|exit       - Exit the program\n"
           "\n"
           "Address and data can be specified in decimal, octal (prefix 0) or hexadecimal (prefix 0x)\n");
}
