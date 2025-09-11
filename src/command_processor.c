#include "command_processor.h"
#include "io_access.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

static int parse_number(const char *str, uintptr_t *value)
{
    if (!str || !*str)
        return -1;
    while (isspace((unsigned char)*str)) str++;
    if (!*str)
        return -1;
    if (*str == '-')
        return -1;
    int base = 10;
    if (*str == '0') {
        str++;
        if (*str == 'x') {
            base = 16;
            str++;
        } else if (*str) {
            base = 8;
        } else {
            *value = 0;
            return 0;
        }
    }
    uintptr_t result = 0;
    while (*str) {
        unsigned digit;
        if (isdigit((unsigned char)*str))
            digit = *str - '0';
        else if (base == 16 && *str >= 'A' && *str <= 'F')
            digit = 10 + (*str - 'A');
        else
            break;

        if (digit >= (unsigned)base)
            return -1;
        if (result > (UINTPTR_MAX - digit) / base)
            return -1;
        result = result * base + digit;
        str++;
    }
    while (isspace((unsigned char)*str)) str++;
    if (*str != '\0')
        return -1;
    *value = result;
    return 0;
}

static void print_value(const char *cmd, uintptr_t addr,
      unsigned long val)
{
  if (!strcmp(cmd, "iorb"))
    printf("0x%02lX\n", val & 0xFF);
  else if (!strcmp(cmd, "iorw"))
    printf("0x%04lX\n", val & 0xFFFF);
  else if (!strcmp(cmd, "iord"))
    printf("0x%08lX\n", val & 0xFFFFFFFF);
}

static void handle_read_command(const char *cmd, const char *arg,
        uint8_t (*read_byte)(uintptr_t),
        uint16_t (*read_word)(uintptr_t),
        uint32_t (*read_dword)(uintptr_t))
{
    uintptr_t addr;
    if (parse_number(arg, &addr)) {
    fprintf(stderr, "Invalid address: %s\n", arg);
    return;
    }
    int ok = 1;
	uint64_t val;
    if (!strcmp(cmd, "iorb")) {
        if (mem_read(addr, 1, &val) == 0)
            print_value(cmd, addr, read_byte(addr));
        else
            ok = 0;
    } else if (!strcmp(cmd, "iorw")) {
        if (mem_read(addr, 2, &val) == 0)
            print_value(cmd, addr, read_word(addr));
        else
            ok = 0;
    } else if (!strcmp(cmd, "iord")) {
        if (mem_read(addr, 4, &val) == 0)
            print_value(cmd, addr, read_dword(addr));
        else
            ok = 0;
    }
    if (!ok) {
        fprintf(stderr, "Read failed at 0x%lx\n", (unsigned long)addr);
    }
}

static void handle_write_command(const char *cmd, const char *arg1,
         const char *arg2,
         void (*write_byte)(uintptr_t, uint8_t),
         void (*write_word)(uintptr_t, uint16_t),
         void (*write_dword)(uintptr_t, uint32_t))
{
  uintptr_t addr, data;

  if (parse_number(arg1, &addr)) {
    fprintf(stderr, "Invalid address: %s\n", arg1);
    return;
  }
  if (parse_number(arg2, &data)) {
    fprintf(stderr, "Invalid data: %s\n", arg2);
    return;
  }

  if (!strcmp(cmd, "iowb")) {
    write_byte(addr, (uint8_t)data);
    printf("Write byte 0x%02lX to address 0x%lX\n", data & 0xFF, addr);
  } else if (!strcmp(cmd, "ioww")) {
    write_word(addr, (uint16_t)data);
    printf("Write word 0x%04lX to address 0x%lX\n", data & 0xFFFF, addr);
  } else if (!strcmp(cmd, "iowd")) {
    write_dword(addr, (uint32_t)data);
    printf("Write dword 0x%08lX to address 0x%lX\n", data & 0xFFFFFFFF, addr);
  }
}

int process_command(const char *line)
{
  char cmd[8], arg1[32], arg2[32];
  int count;

  while (isspace(*line))
    line++;
  if (!*line)
    return 0;
/*
the sizes of commands and arguments were initially taken
with a large margin to avoid calculations, 
the numbers 31 and 63 are the lengths of commands and 
arguments -1, so that the zero sign would fit. Reduced the sizes 
to 8 and 32 respectively, 8 fits our commands, 32 is taken with a margin,
the maximum 64-bit decimal number takes up 20 characters. 
For the beauty of the code, values that are multiples of two are taken
*/
  count = sscanf(line, "%7s %31s %31s", cmd, arg1, arg2);

  if (!strcmp(cmd, "quit") || !strcmp(cmd, "exit"))
    return 1;
  if (!strcmp(cmd, "help")) {
    print_help();
    return 0;
  }

  if (!strcmp(cmd, "iorb") || !strcmp(cmd, "iorw") || !strcmp(cmd, "iord")) {
    if (count < 2) {
      fprintf(stderr, "Missing address argument for %s\n", cmd);
      return 0;
    }
    handle_read_command(cmd, arg1, io_read_byte, io_read_word, io_read_dword);
    return 0;
  }

  if (!strcmp(cmd, "iowb") || !strcmp(cmd, "ioww") || !strcmp(cmd, "iowd")) {
    if (count < 3) {
      fprintf(stderr, "Missing address or data argument for %s\n", cmd);
      return 0;
    }
    handle_write_command(cmd, arg1, arg2, io_write_byte, io_write_word, io_write_dword);
    return 0;
  }

  fprintf(stderr, "Unknown command: %s. Type 'help' for available commands.\n", cmd);
  return 0;
}

void print_help(void)
{
  printf("Available commands:\n"
         " iorb <addr> - Read byte from IO address\n"
         " iorw <addr> - Read word from IO address\n"
         " iord <addr> - Read double word from IO address\n"
         " iowb <addr> <data> - Write byte to IO address\n"
         " ioww <addr> <data> - Write word to IO address\n"
         " iowd <addr> <data> - Write double word to IO address\n"
         " help - Show this help message\n"
         " quit|exit - Exit the program\n"
         "\nAddress and data can be specified in decimal, octal (prefix 0) or hexadecimal (prefix 0x)\n");
}

