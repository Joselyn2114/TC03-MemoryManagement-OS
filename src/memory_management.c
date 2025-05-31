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

  // Get memory for the start block
  mm->start_block = malloc(sizeof(Block));
  if (mm->start_block == NULL) {
    fprintf(stderr, "mm_init: Can't get memory for the start block.\n");
    return EXIT_FAILURE;
  }

  // Init the start block
  mm->start_block->size = size;
  mm->start_block->free = true;
  mm->start_block->name = NULL;
  mm->start_block->next = NULL;
  mm->start_block->prev = NULL;

  return EXIT_SUCCESS;
}

void mm_destroy(MemoryManagement* mm) {
  if (mm == NULL) {
    return;
  }

  // Free the blocks
  Block* current = mm->start_block;
  while (current != NULL) {
    Block* next = current->next;
    if (current->name) {
      free(current->name);
    }
    free(current);
    current = next;
  }
  mm->start_block = NULL;
}

int mm_alloc(MemoryManagement* mm, const char* name, size_t size) {
  Block* block_to_use = mm_find_block(mm, size);

  if (block_to_use == NULL) {
    fprintf(
        stderr, "mm_alloc: Can't find a block of size %zu for %s.\n", size, name
    );
    return EXIT_FAILURE;
  }

  // If we have to split the block
  if (block_to_use->size > size) {
    if (mm_alloc_split(block_to_use, size) == EXIT_FAILURE) {
      return EXIT_FAILURE;
    }
  }

  // Copy the name to the block
  char* name_copy = strdup(name);
  if (name_copy == NULL) {
    fprintf(stderr, "mm_alloc: Can't copy name: %s\n", name);
    return EXIT_FAILURE;
  }

  // Mark the block as used
  block_to_use->free = false;
  block_to_use->name = name_copy;

  return EXIT_SUCCESS;
}

int mm_alloc_split(Block* block_to_use, size_t size) {
  // The size of the block after the split
  size_t rest_size = block_to_use->size - size;

  // If the rest size is too small to create a new block or is zero
  // we just use the current block
  if (rest_size <= sizeof(Block) || rest_size == 0) {
    block_to_use->size = size;
    return EXIT_SUCCESS;
  }

  // Create a new block
  Block* new_block = malloc(sizeof(Block));
  if (new_block == NULL) {
    fprintf(stderr, "mm_alloc_split: Can't get memory for the new block.\n");
    return EXIT_FAILURE;
  }

  // Init the new block
  new_block->free = true;
  new_block->name = NULL;
  new_block->size = rest_size;
  new_block->prev = block_to_use;
  new_block->next = block_to_use->next;

  // Update the current block
  if (block_to_use->next != NULL) {
    block_to_use->next->prev = new_block;
  }

  // Link the new block to the current block
  block_to_use->size = size;
  block_to_use->next = new_block;

  return EXIT_SUCCESS;
}

int mm_realloc(MemoryManagement* mm, const char* name, size_t size) {
  Block* block_to_use = NULL;
  Block* current = mm->start_block;

  // Find a block with the same name
  while (current != NULL) {
    // Check if the block is not free and has the same name
    if (!current->free && current->name != NULL &&
        strcmp(current->name, name) == 0) {
      block_to_use = current;
      break;
    }
    current = current->next;
  }

  // If we didn't find a block with the same name, return an error
  if (block_to_use == NULL) {
    fprintf(
        stderr, "mm_realloc: Can't find a free block with name %s.\n", name
    );
    return EXIT_FAILURE;
  }

  // If the requested size the the same as the current block size
  // we don't need to do anything
  if (size == block_to_use->size) {
    return EXIT_SUCCESS;
  }

  // If the requested size is larger than the current block size
  // and we can grow the block
  if (size > block_to_use->size) {
    if (mm_realloc_grow(mm, block_to_use, size) == EXIT_SUCCESS) {
      block_to_use->size = size;
      return EXIT_SUCCESS;
    }
  }

  // If the requested size is smaller than the current block size
  // and we can shrink the block
  if (size < block_to_use->size) {
    if (mm_realloc_shrink(block_to_use, size) == EXIT_SUCCESS) {
      block_to_use->size = size;
      return EXIT_SUCCESS;
    }
  }

  fprintf(
      stderr, "mm_realloc: Can't find a block of size %zu for %s.\n", size, name
  );

  return EXIT_FAILURE;
}

int mm_realloc_grow(MemoryManagement* mm, Block* block_to_use, size_t size) {
  Block* next_block = block_to_use->next;

  // If the next block is NULL or not free, we can't grow the block
  if (next_block == NULL || !next_block->free) {
    return EXIT_FAILURE;
  }

  bool can_grow = block_to_use->size + next_block->size >= size;

  if (next_block != NULL && next_block->free && can_grow) {
    // The size of the block after the grow
    size_t grow_size = block_to_use->size + next_block->size;

    block_to_use->next = next_block->next;
    if (next_block->next != NULL) {
      next_block->next->prev = block_to_use;
    }

    free(next_block->name);
    free(next_block);

    block_to_use->size = grow_size;

    // If the grow size is larger than the requested size
    if (grow_size <= size) {
      block_to_use->size = size;
      return EXIT_SUCCESS;
    }

    // Create a new block for the remaining size
    size_t rest_size = grow_size - size;
    Block* rest_block = malloc(sizeof(Block));
    if (rest_block == NULL) {
      block_to_use->size = size;
      return EXIT_SUCCESS;
    }

    // Init the new block
    rest_block->size = rest_size;
    rest_block->free = true;
    rest_block->name = NULL;
    rest_block->next = block_to_use->next;
    rest_block->prev = block_to_use;

    // Link the new block to the current block
    if (block_to_use->next != NULL) {
      block_to_use->next->prev = rest_block;
    }

    // Link the current block to the new block
    block_to_use->next = rest_block;
    block_to_use->size = size;

    return EXIT_SUCCESS;
  }

  // Copy the name to a new block
  char* name_copy = strdup(block_to_use->name);
  if (name_copy == NULL) {
    fprintf(
        stderr, "mm_realloc_grow: Can't copy name: %s.\n", block_to_use->name
    );
    return EXIT_FAILURE;
  }

  // Try to allocate a new block with the requested size
  int error = mm_alloc(mm, name_copy, size);

  free(name_copy);

  return error;
}

int mm_realloc_shrink(Block* block_to_use, size_t size) {
  // The size of the block after the shrink
  size_t rest_size = block_to_use->size - size;

  // If the rest size is too small to create a new block or is zero
  // we just use the current block
  if (rest_size < sizeof(Block) || rest_size == 0) {
    block_to_use->size = size;
    return EXIT_SUCCESS;
  }

  // Create a new block for the remaining size
  Block* new_block = malloc(sizeof(Block));
  if (new_block == NULL) {
    fprintf(stderr, "mm_realloc_shrink: Can't get memory for the new block.\n");
    block_to_use->size = size;
    return EXIT_SUCCESS;
  }

  // Init the new block
  new_block->size = rest_size;
  new_block->free = true;
  new_block->name = NULL;
  new_block->next = block_to_use->next;
  new_block->prev = block_to_use;

  // Link the new block to the current block
  if (block_to_use->next != NULL) {
    block_to_use->next->prev = new_block;
  }

  // Link the current block to the new block
  block_to_use->next = new_block;
  block_to_use->size = size;

  return EXIT_SUCCESS;
}

int mm_free(MemoryManagement* mm, const char* name) {
  Block* block_to_use = NULL;
  Block* current = mm->start_block;

  // Find a block with the same name
  while (current != NULL) {
    // Check if the block is not free and has the same name
    if (!current->free && current->name != NULL &&
        strcmp(current->name, name) == 0) {
      block_to_use = current;
      break;
    }
    current = current->next;
  }

  // If we didn't find a block with the same name, return an error
  if (block_to_use == NULL) {
    fprintf(stderr, "mm_free: Can't find a block with name %s.\n", name);
    return EXIT_FAILURE;
  }

  free(block_to_use->name);

  // Mark the block as free
  block_to_use->free = true;
  block_to_use->name = NULL;

  // Join the block with the next and previous blocks if they are free
  mm_free_join(block_to_use);

  return EXIT_SUCCESS;
}

void mm_free_join(Block* block_to_use) {
  // If the next block is free, join it with the current block
  while (block_to_use->next && block_to_use->next->free) {
    Block* next_block = block_to_use->next;
    block_to_use->size += next_block->size;
    block_to_use->next = next_block->next;
    if (next_block->next) next_block->next->prev = block_to_use;
    free(next_block->name);
    free(next_block);
  }

  // If the previous block is free, join it with the current block
  while (block_to_use->prev && block_to_use->prev->free) {
    Block* prev_block = block_to_use->prev;
    prev_block->size += block_to_use->size;
    prev_block->next = block_to_use->next;
    if (block_to_use->next) block_to_use->next->prev = prev_block;
    free(block_to_use->name);
    free(block_to_use);
    block_to_use = prev_block;
  }
}

void mm_print(const MemoryManagement* mm) {
  printf("Memory Management:\n");

  int i = 0;
  Block* current = mm->start_block;

  while (current != NULL) {
    printf("Block: %d, ", i);
    if (!current->free) {
      printf("Name: %s, ", current->name);
    } else {
      printf("Free, ");
    }
    printf("Size: %zu\n", current->size);

    i++;
    current = current->next;
  }
}

int mm_start(MemoryManagement* mm, const char* filename) {
  // Open the file
  FILE* file = fopen(filename, "r");
  if (file == NULL) {
    fprintf(stderr, "mm_start: Can't open file: %s\n", filename);
    return EXIT_FAILURE;
  }

  // Read the file line by line
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), file)) {
    // Skip empty lines and comments
    if (buffer[0] == '\n' || buffer[0] == '#') {
      continue;
    }

    // Parse the command
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

    // Execute the command
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
  (void) mm;
  (void) size;
  fprintf(stderr, "Not implemented: mm_find_block_first_fit.\n");
  return NULL;
}

Block* mm_find_block_best_fit(MemoryManagement* mm, size_t size) {
  (void) mm;
  (void) size;
  fprintf(stderr, "Not implemented: mm_find_block_best_fit.\n");
  return NULL;
}

Block* mm_find_block_worst_fit(MemoryManagement* mm, size_t size) {
  Block* worst_fit_block = NULL;
  Block* current = mm->start_block;

  // Iterate through the blocks to find the worst fit
  while (current != NULL) {
    // Check if the block is free and has enough size
    if (current->free && current->size >= size) {
      if (worst_fit_block == NULL || current->size > worst_fit_block->size) {
        worst_fit_block = current;
      }
    }
    current = current->next;
  }

  // If we didn't find a block, return NULL
  if (worst_fit_block == NULL) {
    fprintf(
        stderr, "mm_find_block_worst_fit: Can't find a block of size %zu.\n",
        size
    );
  }

  return worst_fit_block;
}
