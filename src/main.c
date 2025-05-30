//

#include <stdio.h>
#include <stdlib.h>

#include "memory_management.h"
#include "parser.h"

#define MEMORY_SIZE 1024 * 1024  // 1 MB

int main(int argc, char** argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <file> <best|first|worst>.\n", argv[0]);
    return EXIT_FAILURE;
  }

  StrategyType strategy;
  if (parse_strategy(argv[2], &strategy) != EXIT_SUCCESS) {
    fprintf(stderr, "Unknown strategy: %s\n", argv[2]);
    return EXIT_FAILURE;
  }

  MemoryManagement mm;
  if (mm_init(&mm, strategy, MEMORY_SIZE) != EXIT_SUCCESS) {
    fprintf(stderr, "Can't init memory management.\n");
    return EXIT_FAILURE;
  }

  if (mm_start(&mm, argv[1]) != EXIT_SUCCESS) {
    mm_destroy(&mm);
    return EXIT_FAILURE;
  }

  mm_destroy(&mm);

  return EXIT_SUCCESS;
}
