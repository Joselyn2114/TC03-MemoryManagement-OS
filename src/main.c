//

#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

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

  return EXIT_SUCCESS;
}
