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
    fprintf(stderr, "mm_init: Can't get memory for the start block.\n");
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
    Block* next = current->next;
    if (current->name) {
      free(current->name);
    }
    free(current);
    current = next;
  }
  mm->start = NULL;
}

int mm_alloc(MemoryManagement* mm, const char* name, size_t size) {
  Block* block_to_use = mm_find_block(mm, size);

  if (block_to_use == NULL) {
    fprintf(
        stderr, "mm_alloc: Can't find a block of size %zu for %s.\n", size, name
    );
    return EXIT_FAILURE;
  }

  char* name_copy = strdup(name);
  if (name_copy == NULL) {
    fprintf(stderr, "mm_alloc: Can't copy name: %s\n", name);
    return EXIT_FAILURE;
  }

  size_t save_block_size = block_to_use->size;

  if (save_block_size > size) {
    if (mm_alloc_split(mm, block_to_use, size, save_block_size) ==
        EXIT_FAILURE) {
      free(name_copy);
      return EXIT_FAILURE;
    }
  }

  block_to_use->free = false;
  block_to_use->name = name_copy;

  return EXIT_SUCCESS;
}

int mm_alloc_split(
    MemoryManagement* mm, Block* block_to_use, size_t size,
    size_t save_block_size
) {
  size_t rest_size = save_block_size - size;

  if (rest_size > sizeof(Block)) {
    Block* new_block = malloc(sizeof(Block));
    if (new_block == NULL) {
      fprintf(stderr, "mm_alloc_split: Can't get memory for the new block.\n");
      return EXIT_FAILURE;
    }

    new_block->size = rest_size;
    new_block->free = true;
    new_block->name = NULL;
    new_block->prev = block_to_use;
    new_block->next = block_to_use->next;

    if (block_to_use->next != NULL) {
      block_to_use->next->prev = new_block;
    }

    block_to_use->next = new_block;
    block_to_use->size = size;
  }

  return EXIT_SUCCESS;
}

int mm_realloc(MemoryManagement* mm, const char* name, size_t size) {
  Block* current = mm->start;
  Block* block_to_use = NULL;

  while (current != NULL) {
    if (!current->free && current->name != NULL &&
        strcmp(current->name, name) == 0) {
      block_to_use = current;
      break;
    }
    current = current->next;
  }

  if (block_to_use == NULL) {
    fprintf(
        stderr, "mm_realloc: Can't find a free block with name %s.\n", name
    );
    return EXIT_FAILURE;
  }

  if (size == block_to_use->size) {
    return EXIT_SUCCESS;
  }

  if (size > block_to_use->size) {
    if (mm_realloc_grow(mm, block_to_use, size) == EXIT_SUCCESS) {
      block_to_use->size = size;
      return EXIT_SUCCESS;
    }
  }

  if (size < block_to_use->size) {
    if (mm_realloc_shrink(mm, block_to_use, size) == EXIT_SUCCESS) {
      block_to_use->size = size;
      return EXIT_SUCCESS;
    }
  }

  return EXIT_FAILURE;
}

int mm_realloc_grow(MemoryManagement* mm, Block* block_to_use, size_t size) {
  Block* next_block = block_to_use->next;

  if (next_block != NULL && next_block->free &&
      (block_to_use->size + next_block->size >= size)) {
    size_t grow_size = block_to_use->size + next_block->size;

    block_to_use->next = next_block->next;
    if (next_block->next != NULL) {
      next_block->next->prev = block_to_use;
    }

    free(next_block->name);
    free(next_block);

    block_to_use->size = grow_size;

    if (grow_size > size) {
      size_t rest_size = grow_size - size;
      Block* rest_block = (Block*) malloc(sizeof(Block));

      if (rest_block == NULL) {
        block_to_use->size = size;
        return EXIT_SUCCESS;
      }

      rest_block->size = rest_size;
      rest_block->free = true;
      rest_block->name = NULL;
      rest_block->next = block_to_use->next;
      rest_block->prev = block_to_use;

      if (block_to_use->next != NULL) {
        block_to_use->next->prev = rest_block;
      }

      block_to_use->next = rest_block;
      block_to_use->size = size;
    } else {
      block_to_use->size = size;
    }

    return EXIT_SUCCESS;
  } else {
    char* name_copy = strdup(block_to_use->name);
    if (name_copy == NULL) {
      fprintf(
          stderr, "mm_realloc_grow: Can't copy name: %s.\n", block_to_use->name
      );
      return EXIT_FAILURE;
    }

    int error = mm_alloc(mm, name_copy, size);
    free(name_copy);
    return error;
  }
}

int mm_realloc_shrink(MemoryManagement* mm, Block* block_to_use, size_t size) {
  size_t rest_size = block_to_use->size - size;

  if (rest_size > 0) {
    Block* new_block = (Block*) malloc(sizeof(Block));
    if (new_block == NULL) {
      fprintf(
          stderr, "mm_realloc_shrink: Can't get memory for the new block.\n"
      );
      block_to_use->size = size;
      return EXIT_SUCCESS;
    }

    new_block->size = rest_size;
    new_block->free = true;
    new_block->name = NULL;
    new_block->next = block_to_use->next;
    new_block->prev = block_to_use;

    if (block_to_use->next != NULL) {
      block_to_use->next->prev = new_block;
    }

    block_to_use->next = new_block;
    block_to_use->size = size;
  } else {
    block_to_use->size = size;
  }

  return EXIT_SUCCESS;
}

int mm_free(MemoryManagement* mm, const char* name) {
  Block* current = mm->start;
  Block* block_to_use = NULL;

  while (current != NULL) {
    if (!current->free && current->name != NULL &&
        strcmp(current->name, name) == 0) {
      block_to_use = current;
      break;
    }
    current = current->next;
  }

  if (block_to_use == NULL) {
    fprintf(stderr, "mm_free: Can't find a block with name %s.\n", name);
    return EXIT_FAILURE;
  }

  free(block_to_use->name);

  block_to_use->name = NULL;
  block_to_use->free = true;

  mm_free_join(mm, block_to_use);

  return EXIT_SUCCESS;
}

void mm_free_join(MemoryManagement* mm, Block* block_to_use) {
  if (block_to_use->next != NULL && block_to_use->next->free) {
    Block* next_block = block_to_use->next;
    block_to_use->size += next_block->size;
    block_to_use->next = next_block->next;

    if (next_block->next != NULL) {
      next_block->next->prev = block_to_use;
    }

    free(next_block->name);
    free(next_block);
  }

  if (block_to_use->prev != NULL && block_to_use->prev->free) {
    Block* prev_block = block_to_use->prev;

    prev_block->size += block_to_use->size;
    prev_block->next = block_to_use->next;

    if (block_to_use->next != NULL) {
      block_to_use->next->prev = prev_block;
    }

    free(block_to_use->name);
    free(block_to_use);
  }
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
    fprintf(stderr, "mm_start: Can't open file: %s\n", filename);
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
      fprintf(stderr, "mm_start: Can't parse command: %s.\n", buffer);
      return EXIT_FAILURE;
    }

    if (mm_execute_command(mm, &command) != EXIT_SUCCESS) {
      if (command.name != NULL) {
        free(command.name);
        command.name = NULL;
      }
      fclose(file);
      fprintf(stderr, "mm_start: Can't execute command: %s.\n", buffer);
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
      fprintf(
          stderr, "mm_execute_command: Unknown command type: %d.\n",
          command->type
      );
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
      fprintf(stderr, "mm_find_block: Unknown strategy: %d.\n", mm->strategy);
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
  Block* current = mm->start;
  Block* worst_fit_block = NULL;

  while (current != NULL) {
    if (current->free && current->size >= size) {
      if (worst_fit_block == NULL || current->size > worst_fit_block->size) {
        worst_fit_block = current;
      }
    }
    current = current->next;
  }

  if (worst_fit_block == NULL) {
    fprintf(
        stderr, "mm_find_block_worst_fit: Can't find a block of size %zu.\n",
        size
    );
  }

  return worst_fit_block;
}
