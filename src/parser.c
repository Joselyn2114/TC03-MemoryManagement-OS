//

#include "parser.h"

#include <stdio.h>
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

  fprintf(stderr, "parse_strategy: Unknown strategy: %s.\n", arg);

  return EXIT_FAILURE;
}

int parse_command(char* buffer, Command* command) {
  command->size = 0;
  command->name = NULL;

  char* arg1 = strtok(buffer, " \n");
  if (arg1 == NULL) {
    fprintf(stderr, "parse_command: No command.\n");
    return EXIT_FAILURE;
  }

  CommandType type;
  if (parse_command_type(arg1, &type) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  command->type = type;

  if (command->type == CMD_ALLOC || command->type == CMD_REALLOC) {
    char* arg2 = strtok(NULL, " \n");
    char* arg3 = strtok(NULL, " \n");

    if (arg2 == NULL || arg3 == NULL) {
      fprintf(stderr, "parse_command: Bad command format.\n");
      return EXIT_FAILURE;
    }

    command->size = strtoul(arg3, NULL, 10);
    command->name = strdup(arg2);
    if (command->name == NULL) {
      fprintf(stderr, "parse_command: Can't copy name: %s\n", arg2);
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
  }

  if (command->type == CMD_FREE) {
    char* arg2 = strtok(NULL, " \n");
    if (arg2 == NULL) {
      fprintf(stderr, "parse_command: Bad command format.\n");
      return EXIT_FAILURE;
    }

    command->name = strdup(arg2);
    if (command->name == NULL) {
      fprintf(stderr, "parse_command: Can't copy name: %s.\n", arg2);
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
  }

  if (command->type == CMD_PRINT) {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}

int parse_command_type(const char* arg, CommandType* type) {
  if (strcmp(arg, "ALLOC") == 0) {
    *type = CMD_ALLOC;
    return EXIT_SUCCESS;
  }

  if (strcmp(arg, "REALLOC") == 0) {
    *type = CMD_REALLOC;
    return EXIT_SUCCESS;
  }

  if (strcmp(arg, "FREE") == 0) {
    *type = CMD_FREE;
    return EXIT_SUCCESS;
  }

  if (strcmp(arg, "PRINT") == 0) {
    *type = CMD_PRINT;
    return EXIT_SUCCESS;
  }

  fprintf(stderr, "parse_command_type: Unknown command type: %s.\n", arg);

  return EXIT_FAILURE;
}
