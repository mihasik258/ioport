#include "io_access.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>
#include <string.h>

#define PAGE_SIZE  4096
#define PORT_MASK  0xFFFF

static int  dev_mem_fd = -1;
static bool  has_port_access = false;

static inline bool is_port_address(uintptr_t addr)
{
  return addr <= PORT_MASK;
}

static inline uintptr_t align_to_page(uintptr_t addr)
{
  return addr & ~(PAGE_SIZE - 1);
}

static inline size_t get_page_offset(uintptr_t addr)
{
  return addr & (PAGE_SIZE - 1);
}

static void *map_addr(uintptr_t addr, int prot)
{
  uintptr_t page_base = align_to_page(addr);

  void *map = mmap(NULL, PAGE_SIZE, prot, MAP_SHARED,
       dev_mem_fd, page_base);
  if (map == MAP_FAILED) {
    fprintf(stderr, "Failed to map memory at 0x%lx: %s\n",
      (unsigned long)addr, strerror(errno));
    return NULL;
  }

  return map;
}

static inline uint64_t mem_read(uintptr_t addr, size_t size)
{
  void *map = map_addr(addr, PROT_READ);
  if (!map)
    return 0;

  off_t offset = get_page_offset(addr);
  uint64_t val = 0;

  switch (size) {
  case 1:
    val = *((volatile uint8_t *)((uintptr_t)map + offset));
    break;
  case 2:
    val = *((volatile uint16_t *)((uintptr_t)map + offset));
    break;
  case 4:
    val = *((volatile uint32_t *)((uintptr_t)map + offset));
    break;
  default:
    fprintf(stderr, "Unsupported read size %zu\n", size);
  }

  munmap(map, PAGE_SIZE);
  return val;
}

static inline void mem_write(uintptr_t addr, uint64_t value, size_t size)
{
  void *map = map_addr(addr, PROT_READ | PROT_WRITE);
  if (!map)
    return;

  off_t offset = get_page_offset(addr);

  switch (size) {
  case 1:
    *((volatile uint8_t *)((uintptr_t)map + offset)) = (uint8_t)value;
    break;
  case 2:
    *((volatile uint16_t *)((uintptr_t)map + offset)) = (uint16_t)value;
    break;
  case 4:
    *((volatile uint32_t *)((uintptr_t)map + offset)) = (uint32_t)value;
    break;
  default:
    fprintf(stderr, "Unsupported write size %zu\n", size);
  }

  munmap(map, PAGE_SIZE);
}

bool io_init(void)
{
  if (geteuid() != 0)
    fprintf(stderr, "Warning: Root privileges required for direct hardware access\n");

  if (iopl(3) == 0) {
    has_port_access = true;
  } else {
    fprintf(stderr, "Warning: Failed to get I/O port access: %s\n",
      strerror(errno));
    has_port_access = false;
  }

  dev_mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (dev_mem_fd < 0) {
    fprintf(stderr, "Warning: Failed to open /dev/mem: %s\n",
      strerror(errno));
    return has_port_access;
  }

  return true;
}

void io_cleanup(void)
{
  if (dev_mem_fd >= 0) {
    close(dev_mem_fd);
    dev_mem_fd = -1;
  }
}

uint8_t io_read_byte(uintptr_t addr)
{
  if (is_port_address(addr) && has_port_access)
    return inb(addr);
  return (uint8_t)mem_read(addr, 1);
}

uint16_t io_read_word(uintptr_t addr)
{
  if (is_port_address(addr) && has_port_access)
    return inw(addr);
  return (uint16_t)mem_read(addr, 2);
}

uint32_t io_read_dword(uintptr_t addr)
{
  if (is_port_address(addr) && has_port_access)
    return inl(addr);
  return (uint32_t)mem_read(addr, 4);
}

void io_write_byte(uintptr_t addr, uint8_t value)
{
  if (is_port_address(addr) && has_port_access) {
    outb(value, addr);
    return;
  }
  mem_write(addr, value, 1);
}

void io_write_word(uintptr_t addr, uint16_t value)
{
  if (is_port_address(addr) && has_port_access) {
    outw(value, addr);
    return;
  }
  mem_write(addr, value, 2);
}

void io_write_dword(uintptr_t addr, uint32_t value)
{
  if (is_port_address(addr) && has_port_access) {
    outl(value, addr);
    return;
  }
  mem_write(addr, value, 4);
}

