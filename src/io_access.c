#include "io_access.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>
#include <string.h>

#define PAGE_SIZE 4096
#define PORT_MASK 0xFFFF

static int dev_mem_fd = -1;
static bool has_port_access = false;

static inline bool is_port_address(uintptr_t addr) {
    return addr <= PORT_MASK;
}

static inline uintptr_t align_to_page(uintptr_t addr) {
    return addr & ~(PAGE_SIZE - 1);
}

static inline size_t get_page_offset(uintptr_t addr) {
    return addr & (PAGE_SIZE - 1);
}

bool io_init(void) {
    if (geteuid() != 0) {
        fprintf(stderr, "Warning: Root privileges required for direct hardware access\n");
    }
    
    if (iopl(3) == 0) {
        has_port_access = true;
    } else {
        fprintf(stderr, "Warning: Failed to get I/O port access: %s\n", strerror(errno));
        has_port_access = false;
    }
    
    dev_mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (dev_mem_fd < 0) {
        fprintf(stderr, "Warning: Failed to open /dev/mem: %s\n", strerror(errno));
        return has_port_access; 
    }
    
    return true;
}

void io_cleanup(void) {
    if (dev_mem_fd >= 0) {
        close(dev_mem_fd);
        dev_mem_fd = -1;
    }
}

uint8_t io_read_byte(uintptr_t addr) {
    if (is_port_address(addr) && has_port_access) {
        return inb(addr);
    } else {
        uintptr_t page_base = align_to_page(addr);
        off_t offset = get_page_offset(addr);
        
        void* map = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, dev_mem_fd, page_base);
        if (map == MAP_FAILED) {
            fprintf(stderr, "Failed to map memory: %s\n", strerror(errno));
            return 0;
        }
        
        uint8_t value = *((volatile uint8_t*)((uintptr_t)map + offset));
        munmap(map, PAGE_SIZE);
        
        return value;
    }
}

uint16_t io_read_word(uintptr_t addr) {
    if (is_port_address(addr) && has_port_access) {
        return inw(addr);
    } else {
        uintptr_t page_base = align_to_page(addr);
        off_t offset = get_page_offset(addr);
        
        void* map = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, dev_mem_fd, page_base);
        if (map == MAP_FAILED) {
            fprintf(stderr, "Failed to map memory: %s\n", strerror(errno));
            return 0;
        }
        
        uint16_t value = *((volatile uint16_t*)((uintptr_t)map + offset));
        munmap(map, PAGE_SIZE);
        
        return value;
    }
}

uint32_t io_read_dword(uintptr_t addr) {
    if (is_port_address(addr) && has_port_access) {
        return inl(addr);
    } else {
        uintptr_t page_base = align_to_page(addr);
        off_t offset = get_page_offset(addr);
        
        void* map = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, dev_mem_fd, page_base);
        if (map == MAP_FAILED) {
            fprintf(stderr, "Failed to map memory: %s\n", strerror(errno));
            return 0;
        }
        
        uint32_t value = *((volatile uint32_t*)((uintptr_t)map + offset));
        munmap(map, PAGE_SIZE);
        
        return value;
    }
}

void io_write_byte(uintptr_t addr, uint8_t value) {
    if (is_port_address(addr) && has_port_access) {
        outb(value, addr);
    } else {
        uintptr_t page_base = align_to_page(addr);
        off_t offset = get_page_offset(addr);
        
        void* map = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_mem_fd, page_base);
        if (map == MAP_FAILED) {
            fprintf(stderr, "Failed to map memory: %s\n", strerror(errno));
            return;
        }
        
        *((volatile uint8_t*)((uintptr_t)map + offset)) = value;
        munmap(map, PAGE_SIZE);
    }
}

void io_write_word(uintptr_t addr, uint16_t value) {
    if (is_port_address(addr) && has_port_access) {
        outw(value, addr);
    } else {
        uintptr_t page_base = align_to_page(addr);
        off_t offset = get_page_offset(addr);
        
        void* map = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_mem_fd, page_base);
        if (map == MAP_FAILED) {
            fprintf(stderr, "Failed to map memory: %s\n", strerror(errno));
            return;
        }
        
        *((volatile uint16_t*)((uintptr_t)map + offset)) = value;
        munmap(map, PAGE_SIZE);
    }
}

void io_write_dword(uintptr_t addr, uint32_t value) {
    if (is_port_address(addr) && has_port_access) {
        outl(value, addr);
    } else {
        uintptr_t page_base = align_to_page(addr);
        off_t offset = get_page_offset(addr);
        
        void* map = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_mem_fd, page_base);
        if (map == MAP_FAILED) {
            fprintf(stderr, "Failed to map memory: %s\n", strerror(errno));
            return;
        }
        
        *((volatile uint32_t*)((uintptr_t)map + offset)) = value;
        munmap(map, PAGE_SIZE);
    }
}
