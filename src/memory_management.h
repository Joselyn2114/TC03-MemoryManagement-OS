//

#ifndef MEMORY_MANAGEMENT_H
#define MEMORY_MANAGEMENT_H

#include <stdbool.h>
#include <stddef.h>

#include "strategy.h"

typedef struct {
  bool free;
  char* name;
} Block;

typedef struct {
  StrategyType strategy;
  Block* blocks;
} MemoryManagement;

int mm_init(MemoryManagement* mm, StrategyType strategy);

void mm_destroy(MemoryManagement* mm);

int mm_alloc(MemoryManagement* mm, size_t size, const char* name);

int mm_realloc(MemoryManagement* mm, size_t index, size_t size);

int mm_free(MemoryManagement* mm, size_t index);

void mm_print(const MemoryManagement* mm);

int mm_start(MemoryManagement* mm, const char* filename);

#endif  // MEMORY_MANAGEMENT_H
