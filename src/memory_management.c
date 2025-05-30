//

#include "memory_management.h"

#include <stdio.h>
#include <stdlib.h>

#define BLOCK_COUNT 1024
#define BLOCK_SIZE 1024

int mm_init(MemoryManagement* mm, StrategyType strategy) {
  mm->strategy = strategy;
  mm->blocks = calloc(BLOCK_COUNT, sizeof(Block));
  if (mm->blocks == NULL) {
    return EXIT_FAILURE;
  }

  for (size_t i = 0; i < BLOCK_COUNT; ++i) {
    mm->blocks[i].free = true;
    mm->blocks[i].name = NULL;
  }

  return EXIT_SUCCESS;
}

void mm_destroy(MemoryManagement* mm) {
  for (size_t i = 0; i < BLOCK_COUNT; ++i) {
    free(mm->blocks[i].name);
  }

  free(mm->blocks);
}

int mm_alloc(MemoryManagement* mm, size_t size, const char* name) {
  fprintf(stderr, "Not implemented.\n");
  return EXIT_FAILURE;
}

int mm_realloc(MemoryManagement* mm, size_t index, size_t new_size) {
  fprintf(stderr, "Not implemented.\n");
  return EXIT_FAILURE;
}

int mm_free(MemoryManagement* mm, size_t index) {
  fprintf(stderr, "Not implemented.\n");
  return EXIT_FAILURE;
}

void mm_print(const MemoryManagement* mm) {
  printf("Memory Management:\n");
  for (size_t i = 0; i < BLOCK_COUNT; ++i) {
    if (!mm->blocks[i].free) {
      printf("Block %zu: Name: %s\n", i, mm->blocks[i].name);
    }
  }
}
