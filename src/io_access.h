#ifndef IO_ACCESS_H
#define IO_ACCESS_H

#include <stdint.h>
#include <stdbool.h>

bool io_init(void);

void io_cleanup(void);

uint8_t io_read_byte(uintptr_t addr);

uint16_t io_read_word(uintptr_t addr);

uint32_t io_read_dword(uintptr_t addr);

void io_write_byte(uintptr_t addr, uint8_t value);

void io_write_word(uintptr_t addr, uint16_t value);

void io_write_dword(uintptr_t addr, uint32_t value);

#endif /* IO_ACCESS_H */
