//

#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

typedef enum {
  //
  CMD_ALLOC,
  CMD_REALLOC,
  CMD_FREE,
  CMD_PRINT
} CommandType;

typedef struct {
  CommandType type;
  char* name;
  size_t size;
} Command;

#endif  // COMMAND_H
