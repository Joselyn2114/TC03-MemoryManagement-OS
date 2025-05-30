//

#include "memory_management.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command.h"
#include "parser.h"

int mm_init(MemoryManagement* mm, StrategyType strategy, size_t size) {
  mm->strategy = strategy;
  mm->size = size;

  mm->start = malloc(sizeof(Block));
  if (mm->start == NULL) {
    fprintf(stderr, "Can't get memory for the start.\n");
    return EXIT_FAILURE;
  }

  mm->start->size = size;
  mm->start->free = true;
  mm->start->name = NULL;
  mm->start->next = NULL;
  mm->start->prev = NULL;

  return EXIT_SUCCESS;
}

void mm_destroy(MemoryManagement* mm) {
  if (mm == NULL) {
    return;
  }

  Block* current = mm->start;
  while (current != NULL) {
    if (current->name != NULL) {
      free(current->name);
      current->name = NULL;
    }
    current = current->next;
  }

  mm->start = NULL;
}

int mm_alloc(MemoryManagement* mm, const char* name, size_t user_size) {
  fprintf(stderr, "Not implemented: mm_alloc.\n");
  return EXIT_FAILURE;
}

int mm_realloc(MemoryManagement* mm, const char* name, size_t new_user_size) {
  fprintf(stderr, "Not implemented: mm_realloc.\n");
  return EXIT_FAILURE;
}

int mm_free(MemoryManagement* mm, const char* name) {
  fprintf(stderr, "Not implemented: mm_free.\n");
  return EXIT_FAILURE;
}

void mm_print(const MemoryManagement* mm) {
  printf("Memory Management:\n");

  int i = 0;
  Block* current = mm->start;

  while (current != NULL) {
    if (!current->free) {
      printf("Block: %d, ", i);
      printf("Name: %s, ", current->name ? current->name : "NULL");
      printf("Size: %zu\n", current->size);
    }

    i++;
    current = current->next;
  }
}

int mm_start(MemoryManagement* mm, const char* filename) {
  FILE* file = fopen(filename, "r");
  if (file == NULL) {
    fprintf(stderr, "Can't open file: %s\n", filename);
    return EXIT_FAILURE;
  }

  char buffer[256];
  while (fgets(buffer, sizeof(buffer), file)) {
    if (buffer[0] == '\n' || buffer[0] == '#') {
      continue;
    }

    Command command;
    if (parse_command(buffer, &command) != EXIT_SUCCESS) {
      if (command.name != NULL) {
        free(command.name);
        command.name = NULL;
      }
      fclose(file);
      fprintf(stderr, "Can't parse command: %s.\n", buffer);
      return EXIT_FAILURE;
    }

    if (mm_execute_command(mm, &command) != EXIT_SUCCESS) {
      if (command.name != NULL) {
        free(command.name);
        command.name = NULL;
      }
      fclose(file);
      fprintf(stderr, "Can't execute command: %s.\n", buffer);
      return EXIT_FAILURE;
    }

    if (command.name != NULL) {
      free(command.name);
      command.name = NULL;
    }
  }

  fclose(file);

  return EXIT_SUCCESS;
}

int mm_execute_command(MemoryManagement* mm, const Command* command) {
  switch (command->type) {
    case CMD_ALLOC:
      return mm_alloc(mm, command->name, command->size);
    case CMD_REALLOC:
      return mm_realloc(mm, command->name, command->size);
    case CMD_FREE:
      return mm_free(mm, command->name);
    case CMD_PRINT:
      mm_print(mm);
      return EXIT_SUCCESS;
    default:
      fprintf(stderr, "Unknown command type: %d.\n", command->type);
      return EXIT_FAILURE;
  }
}

Block* mm_find_block(MemoryManagement* mm, size_t requested_total_size) {
  switch (mm->strategy) {
    case STRATEGY_FIRST:
      return mm_find_block_first_fit(mm, requested_total_size);
    case STRATEGY_BEST:
      return mm_find_block_best_fit(mm, requested_total_size);
    case STRATEGY_WORST:
      return mm_find_block_worst_fit(mm, requested_total_size);
    default:
      fprintf(stderr, "Unknown strategy: %d.\n", mm->strategy);
      return NULL;
  }
}

Block* mm_find_block_first_fit(MemoryManagement* mm, size_t size) {
  fprintf(stderr, "Not implemented: mm_find_block_first_fit.\n");
  return NULL;
}

Block* mm_find_block_best_fit(MemoryManagement* mm, size_t size) {
  fprintf(stderr, "Not implemented: mm_find_block_best_fit.\n");
  return NULL;
}

Block* mm_find_block_worst_fit(MemoryManagement* mm, size_t size) {
  fprintf(stderr, "Not implemented: mm_find_block_worst_fit.\n");
  return NULL;
}
