//

#ifndef MEMORY_MANAGEMENT_H
#define MEMORY_MANAGEMENT_H

#include <stdbool.h>
#include <stddef.h>

#include "command.h"
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

int mm_alloc(MemoryManagement* mm, const char* name, size_t size);

int mm_realloc(MemoryManagement* mm, const char* name, size_t size);

int mm_free(MemoryManagement* mm, const char* name);

void mm_print(const MemoryManagement* mm);

int mm_start(MemoryManagement* mm, const char* filename);

int mm_execute_command(MemoryManagement* mm, const Command* command);

#endif  // MEMORY_MANAGEMENT_H
