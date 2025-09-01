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
	long num;

	if (!str || *str == '\0')
		return -1;

	if (strncmp(str, "0x", 2) == 0)
		num = strtoul(str, &endptr, 16);
	else if (str[0] == '0' && isdigit(str[1]))
		num = strtoul(str, &endptr, 8);
	else
		num = strtoul(str, &endptr, 10);

	if (*endptr != '\0')
		return -1;

	*value = (uintptr_t)num;
	return 0;
}

static void handle_read_command(const char *cmd, const char *arg,
			       uint8_t(*read_byte)(uintptr_t),
			       uint16_t(*read_word)(uintptr_t),
			       uint32_t(*read_dword)(uintptr_t))
{
	uintptr_t addr;

	if (parse_number(arg, &addr) != 0) {
		fprintf(stderr, "Invalid address: %s\n", arg);
		return;
	}

	if (strcmp(cmd, "iorb") == 0) {
		uint8_t value = read_byte(addr);
		printf("0x%02X\n", value);
	} else if (strcmp(cmd, "iorw") == 0) {
		uint16_t value = read_word(addr);
		printf("0x%04X\n", value);
	} else if (strcmp(cmd, "iord") == 0) {
		uint32_t value = read_dword(addr);
		printf("0x%08X\n", value);
	}
}

static void handle_write_command(const char *cmd, const char *arg1, const char *arg2,
				 void(*write_byte)(uintptr_t, uint8_t),
				 void(*write_word)(uintptr_t, uint16_t),
				 void(*write_dword)(uintptr_t, uint32_t))
{
	uintptr_t addr;
	uintptr_t data;

	if (parse_number(arg1, &addr) != 0) {
		fprintf(stderr, "Invalid address: %s\n", arg1);
		return;
	}

	if (parse_number(arg2, &data) != 0) {
		fprintf(stderr, "Invalid data: %s\n", arg2);
		return;
	}

	if (strcmp(cmd, "iowb") == 0) {
		write_byte(addr, (uint8_t)data);
		printf("Write byte 0x%02X to address 0x%lX\n", (uint8_t)data, addr);
	} else if (strcmp(cmd, "ioww") == 0) {
		write_word(addr, (uint16_t)data);
		printf("Write word 0x%04X to address 0x%lX\n", (uint16_t)data, addr);
	} else if (strcmp(cmd, "iowd") == 0) {
		write_dword(addr, (uint32_t)data);
		printf("Write dword 0x%08X to address 0x%lX\n", (uint32_t)data, addr);
	}
}

int process_command(const char *line)
{
	char cmd[32];
	char arg1[64];
	char arg2[64];
	int count;

	while (isspace(*line))
		line++;

	if (*line == '\0')
		return 0;

	count = sscanf(line, "%31s %63s %63s", cmd, arg1, arg2);

	if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0)
		return 1;

	if (strcmp(cmd, "help") == 0) {
		print_help();
		return 0;
	}

	if (strcmp(cmd, "iorb") == 0 || strcmp(cmd, "iorw") == 0 || strcmp(cmd, "iord") == 0) {
		if (count < 2) {
			fprintf(stderr, "Missing address argument for %s\n", cmd);
			return 0;
		}
		handle_read_command(cmd, arg1, io_read_byte, io_read_word, io_read_dword);
		return 0;
	}

	if (strcmp(cmd, "iowb") == 0 || strcmp(cmd, "ioww") == 0 || strcmp(cmd, "iowd") == 0) {
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
