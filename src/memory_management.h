//

#ifndef MEMORY_MANAGEMENT_H
#define MEMORY_MANAGEMENT_H

#include <stdbool.h>
#include <stddef.h>

#include "command.h"
#include "strategy.h"

typedef struct Block {
  bool free;
  char* name;
  size_t size;
  struct Block* next;
  struct Block* prev;
} Block;

typedef struct {
  StrategyType strategy;
  size_t size;
  Block* start;
} MemoryManagement;

int mm_init(MemoryManagement* mm, StrategyType strategy, size_t size);

void mm_destroy(MemoryManagement* mm);

int mm_alloc(MemoryManagement* mm, const char* name, size_t size);

int mm_realloc(MemoryManagement* mm, const char* name, size_t size);

int mm_free(MemoryManagement* mm, const char* name);

void mm_print(const MemoryManagement* mm);

int mm_start(MemoryManagement* mm, const char* filename);

int mm_execute_command(MemoryManagement* mm, const Command* command);

Block* mm_find_block(MemoryManagement* mm, size_t size);

Block* mm_find_block_first_fit(MemoryManagement* mm, size_t size);

Block* mm_find_block_best_fit(MemoryManagement* mm, size_t size);

Block* mm_find_block_worst_fit(MemoryManagement* mm, size_t size);

#endif  // MEMORY_MANAGEMENT_H
