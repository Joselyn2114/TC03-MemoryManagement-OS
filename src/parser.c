//

#include "parser.h"

#include <stdlib.h>
#include <string.h>

int parse_strategy(const char* arg, StrategyType* strategy) {
  if (strcmp(arg, "best") == 0) {
    *strategy = STRATEGY_BEST;
    return EXIT_SUCCESS;
  }

  if (strcmp(arg, "first") == 0) {
    *strategy = STRATEGY_FIRST;
    return EXIT_SUCCESS;
  }

  if (strcmp(arg, "worst") == 0) {
    *strategy = STRATEGY_WORST;
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
