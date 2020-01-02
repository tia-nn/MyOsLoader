#ifndef __MEMORY_MAP_H
#define __MEMORY_MAP_H

#include <stdint.h>

#include <Uefi.h>

typedef struct MemoryMap {
    uint64_t memory_map_size;
    EFI_MEMORY_DESCRIPTOR *memory_map;
    uint64_t map_key;
    uint64_t descriptor_size;
    uint32_t descriptor_version;
} MemoryMap;



#endif