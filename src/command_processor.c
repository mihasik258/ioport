#include "command_processor.h"
#include "io_access.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

static int parse_number(const char *str, uintptr_t *value)
{
  char *endptr;
  unsigned long num;

  if (!str || !*str)
    return -1;

  if (!strncmp(str, "0x", 2))
    num = strtoul(str, &endptr, 16);
  else if (str[0] == '0' && isdigit(str[1]))
    num = strtoul(str, &endptr, 8);
  else
    num = strtoul(str, &endptr, 10);

  if (*endptr)
    return -1;

  *value = (uintptr_t)num;
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

  if (!strcmp(cmd, "iorb"))
    print_value(cmd, addr, read_byte(addr));
  else if (!strcmp(cmd, "iorw"))
    print_value(cmd, addr, read_word(addr));
  else if (!strcmp(cmd, "iord"))
    print_value(cmd, addr, read_dword(addr));
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
  char cmd[32], arg1[64], arg2[64];
  int count;

  while (isspace(*line))
    line++;
  if (!*line)
    return 0;

  count = sscanf(line, "%31s %63s %63s", cmd, arg1, arg2);

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

