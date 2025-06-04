#include "memory_management.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command.h"
#include "parser.h"


/**************************************************************************************************
 * mm_init
 *
 *  Inicializa la estructura MemoryManagement. Reserva un bloque grande de 'size' bytes con malloc,
 *  y crea el primer Block, marcándolo como libre (free = true), offset = 0, size = size total.
 *
 *  Parámetros:
 *    - mm: puntero a MemoryManagement (no debe ser NULL)
 *    - strategy: algoritmo de asignación (FIRST, BEST, WORST)
 *    - size: tamaño total (en bytes) que pedimos al SO
 *
 *  Retorna:
 *    - EXIT_SUCCESS si todo salió bien
 *    - EXIT_FAILURE si malloc de memory_region falla o no se pudo crear el primer Block
 */
int mm_init(MemoryManagement* mm, StrategyType strategy, size_t size) {
  if (mm == NULL) {
    fprintf(stderr, "mm_init: puntero mm NULL.\n");
    return EXIT_FAILURE;
  }

  mm->strategy = strategy;
  mm->total_size = size;

  // 1) Pedimos el bloque de tamaño 'size' al SO:
  mm->memory_region = malloc(size);
  if (mm->memory_region == NULL) {
    fprintf(stderr, "mm_init: No se pudo reservar %zu bytes.\n", size);
    return EXIT_FAILURE;
  }

  // 2) Creamos el nodo inicial que representa TODO el bloque libre:
  Block* initial = (Block*) malloc(sizeof(Block));
  if (initial == NULL) {
    fprintf(stderr, "mm_init: No se pudo reservar memoria para el primer Block.\n");
    free(mm->memory_region);
    return EXIT_FAILURE;
  }

  initial->free   = true;
  initial->name   = NULL;
  initial->size   = size;
  initial->offset = 0;      // empieza en el primer byte de memory_region
  initial->next   = NULL;
  initial->prev   = NULL;

  mm->start_block = initial;
  return EXIT_SUCCESS;
}

/**************************************************************************************************
 * mm_destroy
 *
 *  Libera todos los bloques de metadata (lista enlazada) y libera el bloque grande (memory_region).
 *
 *  Parámetros:
 *    - mm: puntero a MemoryManagement; si es NULL, sale sin hacer nada.
 */
void mm_destroy(MemoryManagement* mm) {
  if (mm == NULL) {
    return;
  }

  // 1) Recorrer la lista y liberar cada Block:
  Block* current = mm->start_block;
  while (current != NULL) {
    Block* next = current->next;
    if (current->name != NULL) {
      free(current->name);
    }
    free(current);
    current = next;
  }

  // 2) Liberar la región de datos:
  free(mm->memory_region);
  mm->memory_region = NULL;
  mm->start_block   = NULL;
}

/**************************************************************************************************
 * mm_find_block_first_fit
 *
 *  Recorre la lista desde el inicio y devuelve el PRIMER bloque libre con size >= requested_size.
 *  Si no encuentra, devuelve NULL.
 */
Block* mm_find_block_first_fit(MemoryManagement* mm, size_t requested_size) {
  Block* current = mm->start_block;
  while (current != NULL) {
    if (current->free && current->size >= requested_size) {
      return current;
    }
    current = current->next;
  }
  // No se encontró bloque suficiente
  return NULL;
}

/**************************************************************************************************
 * mm_find_block_best_fit
 *
 *  Recorre toda la lista y guarda el bloque libre con (size >= requested_size) 
 *  y con el size **mínimo** posible (entre los que cumplen).
 *  Si no hay ninguno, devuelve NULL.
 */
Block* mm_find_block_best_fit(MemoryManagement* mm, size_t requested_size) {
  Block* current = mm->start_block;
  Block* best    = NULL;

  while (current != NULL) {
    if (current->free && current->size >= requested_size) {
      if (best == NULL || current->size < best->size) {
        best = current;
      }
    }
    current = current->next;
  }

  return best;
}

/**************************************************************************************************
 * mm_find_block_worst_fit
 *
 *  Recorre la lista y guarda el bloque libre con (size >= requested_size) 
 *  y con el size **máximo** posible (entre los que cumplen).
 *  Si no hay ninguno, devuelve NULL.
 */
Block* mm_find_block_worst_fit(MemoryManagement* mm, size_t requested_size) {
  Block* current     = mm->start_block;
  Block* worst_fit   = NULL;

  while (current != NULL) {
    if (current->free && current->size >= requested_size) {
      if (worst_fit == NULL || current->size > worst_fit->size) {
        worst_fit = current;
      }
    }
    current = current->next;
  }

  return worst_fit;
}

/**************************************************************************************************
 * mm_find_block
 *
 *  Según el strategy almacenado en mm->strategy invoca a la función adecuada:
 *    - FIRST -> mm_find_block_first_fit
 *    - BEST  -> mm_find_block_best_fit
 *    - WORST -> mm_find_block_worst_fit
 */
Block* mm_find_block(MemoryManagement* mm, size_t requested_size) {
  switch (mm->strategy) {
    case STRATEGY_FIRST:
      return mm_find_block_first_fit(mm, requested_size);
    case STRATEGY_BEST:
      return mm_find_block_best_fit(mm, requested_size);
    case STRATEGY_WORST:
      return mm_find_block_worst_fit(mm, requested_size);
    default:
      fprintf(stderr, "mm_find_block: Estrategia desconocida: %d.\n", mm->strategy);
      return NULL;
  }
}

/**************************************************************************************************
 * mm_alloc_split
 *
 *  Divide el bloque block_to_use en 2 partes si el remanente (block_to_use->size - size) 
 *  es mayor que sizeof(Block), para mantener espacio libre. De lo contrario, reasigna 
 *  block_to_use->size = size y no crea bloque nuevo.
 *
 *  Parámetros:
 *    - block_to_use: apuntador a un bloque libre con size >= 'size'
 *    - size: tamaño solicitado por mm_alloc (en bytes)
 *
 *  Retorna:
 *    - EXIT_SUCCESS si pudo (o no necesitó dividir)
 *    - EXIT_FAILURE si malloc para el nuevo bloque falla.
 */
int mm_alloc_split(Block* block_to_use, size_t size) {
  size_t rest_size = block_to_use->size - size;

  // Si el remanente es demasiado pequeño para crear un Block (metadata), no dividimos:
  if (rest_size <= sizeof(Block) || rest_size == 0) {
    block_to_use->size = size;
    return EXIT_SUCCESS;
  }

  // Creamos el bloque nuevo para el remanente:
  Block* new_block = (Block*) malloc(sizeof(Block));
  if (new_block == NULL) {
    fprintf(stderr, "mm_alloc_split: No se pudo reservar memoria para nuevo bloque.\n");
    return EXIT_FAILURE;
  }

  // Inicializamos el bloque “libre” resultante:
  new_block->free   = true;
  new_block->name   = NULL;
  new_block->size   = rest_size;
  new_block->offset = block_to_use->offset + size;  // justo después del bloque original
  new_block->prev   = block_to_use;
  new_block->next   = block_to_use->next;

  // Si había un siguiente, ajustamos su 'prev':
  if (block_to_use->next != NULL) {
    block_to_use->next->prev = new_block;
  }

  // Ajustamos el bloque original para que ocupe EXACTAMENTE 'size' bytes:
  block_to_use->size = size;
  block_to_use->next = new_block;

  return EXIT_SUCCESS;
}

/**************************************************************************************************
 * mm_alloc
 *
 *  1) Busca un bloque libre con size >= size, según strategy.
 *  2) Si no lo encuentra, imprime error y devuelve EXIT_FAILURE.
 *  3) Si el bloque encontrado tiene size > size, llama a mm_alloc_split para crear remanente.
 *  4) Duplica el nombre de la variable y lo asigna a block_to_use->name.
 *  5) Marca block_to_use->free = false.
 *  6) Rellena la parte de memoria (memory_region + offset) con el primer carácter del nombre.
 *
 *  Parámetros:
 *    - mm: puntero a MemoryManagement
 *    - name: cadena con el nombre de variable (p.ej. "A", "foo", etc.)
 *    - size: cuántos bytes queremos reservar
 *
 *  Retorna:
 *    - EXIT_SUCCESS en caso de éxito
 *    - EXIT_FAILURE en caso de no poder asignar o de error interno
 */
int mm_alloc(MemoryManagement* mm, const char* name, size_t size) {
  if (size == 0) {
    fprintf(stderr, "mm_alloc: Tamaño 0 no válido para %s.\n", name);
    return EXIT_FAILURE;
  }

  Block* block_to_use = mm_find_block(mm, size);
  if (block_to_use == NULL) {
    fprintf(stderr,
            "mm_alloc: No se encontró bloque suficiente (se solicitó %zu bytes) para %s.\n",
            size, name);
    return EXIT_FAILURE;
  }

  // 1) Si el bloque es más grande, dividimos:
  if (block_to_use->size > size) {
    if (mm_alloc_split(block_to_use, size) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
  }

  // 2) Guardar el nombre (metadata):
  block_to_use->name = strdup(name);
  if (block_to_use->name == NULL) {
    fprintf(stderr, "mm_alloc: No se pudo duplicar el nombre: %s.\n", name);
    return EXIT_FAILURE;
  }

  // 3) Marca como ocupado:
  block_to_use->free = false;

  // 4) Rellenar con el primer carácter de 'name'
  memset(
    (char*)mm->memory_region + block_to_use->offset,
    name[0],
    block_to_use->size
  );

  return EXIT_SUCCESS;
}

/**************************************************************************************************
 * mm_realloc_shrink
 *
 *  Si el nuevo tamaño 'size' es menor que block_to_use->size, creamos un bloque libre
 *  con el remanente (rest_size = old_size - size) y ajustamos block_to_use->size = size.
 *  Se conserva el mismo offset y nombre. Tras el cambio, se rellena (memset) la parte 
 *  ocupada con el primer carácter de block_to_use->name para “llenar” la nueva zona.
 *
 *  Parámetros:
 *    - block_to_use: bloque ocupado que vamos a “achicar”
 *    - size: nuevo tamaño (menor que block_to_use->size)
 *
 *  Retorna:
 *    - EXIT_SUCCESS (aunque malloc para el nuevo bloque falle, igual reducimos size)
 *    - EXIT_FAILURE solo si no se pudo malloc para metadata (pero en este caso igual hacemos shrink)
 */
int mm_realloc_shrink(Block* block_to_use, size_t size) {
  size_t old_size   = block_to_use->size;
  size_t rest_size  = old_size - size;

  // Si el remanente es demasiado pequeño, simplemente dejamos el bloque  
  if (rest_size < sizeof(Block) || rest_size == 0) {
    block_to_use->size = size;

    // Rellenamos la parte usada con el nombre
    memset(
      (char*)((size_t)0), // placeholder: será rellenado por el llamador
      0,
      0
    );
    return EXIT_SUCCESS;
  }

  // Creamos el nuevo bloque libre (para el remanente):
  Block* new_block = (Block*) malloc(sizeof(Block));
  if (new_block == NULL) {
    fprintf(stderr, "mm_realloc_shrink: No se pudo reservar memoria para el remanente.\n");
    block_to_use->size = size; // Reduce de todos modos
    return EXIT_SUCCESS;
  }

  // Inicializamos el bloque remanente:
  new_block->size   = rest_size;
  new_block->free   = true;
  new_block->name   = NULL;
  new_block->offset = block_to_use->offset + size; // donde termina el bloque original
  new_block->next   = block_to_use->next;
  new_block->prev   = block_to_use;

  // Ajustamos enlaces:
  if (block_to_use->next != NULL) {
    block_to_use->next->prev = new_block;
  }
  block_to_use->next = new_block;

  // Ajustamos tamaño del bloque original:
  block_to_use->size = size;

  return EXIT_SUCCESS;
}

/**************************************************************************************************
 * mm_realloc_grow
 *
 *  Intenta expandir block_to_use bajo dos casos:
 *   a) Si el siguiente bloque existe, está libre y la suma de ambos >= size, 
 *      hacemos join con next_block, liberamos su metadata, y si sobra espacio, 
 *      creamos remanente con mm_realloc_shrink(…) sobre el bloque combinado.
 *
 *   b) Si no hay bloque contiguo libre o no alcanza, devolvemos EXIT_FAILURE 
 *      para que el llamador llame a mm_alloc(...) en otro lugar (simula fuga).
 *
 *  Parámetros:
 *    - mm: estructura completa
 *    - block_to_use: bloque ocupado a expandir
 *    - size: nuevo tamaño solicitado
 *
 *  Retorna:
 *    - EXIT_SUCCESS si logró hacer grow en sitio
 *    - EXIT_FAILURE en cualquier otro caso (que el llamador resuelva clonando en otro bloque)
 */
int mm_realloc_grow(MemoryManagement* mm, Block* block_to_use, size_t size) {
  Block* next_block = block_to_use->next;
  if (next_block == NULL || !next_block->free) {
    // No se puede expandir en sitio
    return EXIT_FAILURE;
  }

  size_t combined_size = block_to_use->size + next_block->size;
  if (combined_size < size) {
    // Aunque sea libre, no alcanza para satisfacer el tamaño
    return EXIT_FAILURE;
  }

  // 1) Unir block_to_use con next_block:
  block_to_use->size = combined_size;
  block_to_use->next = next_block->next;
  if (next_block->next != NULL) {
    next_block->next->prev = block_to_use;
  }
  // Liberamos la metadata del next_block
  if (next_block->name != NULL) free(next_block->name);
  free(next_block);

  // 2) Si la unión es EXACTA (combined_size == size), devolvemos:
  if (combined_size == size) {
    // Rellenar con la primera letra del nombre:
    memset(
      (char*)mm->memory_region + block_to_use->offset,
      block_to_use->name[0],
      block_to_use->size
    );
    return EXIT_SUCCESS;
  }


  // Ajustar temporalmente el size a 'size' para la función shrink:
  block_to_use->size = size;
  size_t rest_size = combined_size - size;

  // Crear bloque remanente (igual a mm_realloc_shrink):
  Block* rest_block = (Block*) malloc(sizeof(Block));
  if (rest_block == NULL) {
    // Si falla el malloc, al menos dejamos al bloque con el nuevo tamaño:
    block_to_use->size = size;
    // Rellenamos con el nombre
    memset(
      (char*)mm->memory_region + block_to_use->offset,
      block_to_use->name[0],
      block_to_use->size
    );
    return EXIT_SUCCESS;
  }

  rest_block->size   = rest_size;
  rest_block->free   = true;
  rest_block->name   = NULL;
  rest_block->offset = block_to_use->offset + size;
  rest_block->next   = block_to_use->next;
  rest_block->prev   = block_to_use;

  if (block_to_use->next != NULL) {
    block_to_use->next->prev = rest_block;
  }
  block_to_use->next = rest_block;

  // Ajustamos el tamaño final del bloque:
  // (ya lo habíamos puesto a 'size')
  // Rellenamos la parte ocupada con el primer carácter:
  memset(
    (char*)mm->memory_region + block_to_use->offset,
    block_to_use->name[0],
    block_to_use->size
  );

  return EXIT_SUCCESS;
}

/**************************************************************************************************
 * mm_realloc
 *
 *  1) Busca en la lista el bloque con nombre == name y free == false.
 *     Si no existe, devuelve error.
 *  2) Si size == block->size, no hace nada.
 *  3) Si size > block->size:
 *      - Llama a mm_realloc_grow. Si da EXIT_SUCCESS, asigna block->size = size; 
 *        de lo contrario, “simula fuga”: duplica nombre y llama a mm_alloc en otro lado. 
 *        Si mm_alloc tiene éxito, libera el nombre duplicado y no toca el bloque viejo.
 *  4) Si size < block->size, llama a mm_realloc_shrink.
 */
int mm_realloc(MemoryManagement* mm, const char* name, size_t size) {
  Block* block_to_use = NULL;
  Block* current      = mm->start_block;

  // 1) Encontrar el bloque con el mismo name:
  while (current != NULL) {
    if (!current->free && current->name != NULL && strcmp(current->name, name) == 0) {
      block_to_use = current;
      break;
    }
    current = current->next;
  }
  if (block_to_use == NULL) {
    fprintf(stderr, "mm_realloc: No se encontró bloque con nombre %s.\n", name);
    return EXIT_FAILURE;
  }

  // 2) Si el tamaño es el mismo, no hacemos nada:
  if (size == block_to_use->size) {
    return EXIT_SUCCESS;
  }

  // 3) Si queremos crecer:
  if (size > block_to_use->size) {
    if (mm_realloc_grow(mm, block_to_use, size) == EXIT_SUCCESS) {
      // mm_realloc_grow ya rellenó con el nombre
      block_to_use->size = size;
      return EXIT_SUCCESS;
    }

    // Si no pudimos crecer en sitio, “simulamos fuga”:
    // Duplicamos el nombre y llamamos a mm_alloc.... (si falla, devolvemos error)
    char* name_copy = strdup(block_to_use->name);
    if (name_copy == NULL) {
      fprintf(stderr, "mm_realloc: No se pudo duplicar nombre para fuga: %s.\n", name);
      return EXIT_FAILURE;
    }
    int err = mm_alloc(mm, name_copy, size);
    free(name_copy);
    if (err != EXIT_SUCCESS) {
      // No pudo asignar en otro bloque
      return EXIT_FAILURE;
    }
    // Devolvemos EXIT_SUCCESS porque dejamos el bloque viejo tal como estaba (fuga real)
    return EXIT_SUCCESS;
  }

  // 4) Si queremos achicar:
  if (size < block_to_use->size) {
    if (mm_realloc_shrink(block_to_use, size) == EXIT_SUCCESS) {
      // Rellenamos la parte ocupada con el nombre (primera letra)
      if (block_to_use->name != NULL) {
        memset(
          (char*)mm->memory_region + block_to_use->offset,
          block_to_use->name[0],
          block_to_use->size
        );
      }
      return EXIT_SUCCESS;
    }
    // Si falla shrink, igual redujo el size:
    return EXIT_SUCCESS;
  }

  // Nunca debería llegar aquí:
  fprintf(stderr, "mm_realloc: Caso inesperado para %s.\n", name);
  return EXIT_FAILURE;
}

/**************************************************************************************************
 * mm_free_join
 *
 *  Después de liberar un bloque (mm_free ha marcado free = true), 
 *  se une con bloques vecinos libres (tanto siguiente como anterior).
 */
void mm_free_join(Block* block_to_use) {
  // 1) Si el siguiente bloque está libre, lo fusionamos:
  while (block_to_use->next && block_to_use->next->free) {
    Block* next_block = block_to_use->next;

    // Aumentamos el tamaño del bloque actual:
    block_to_use->size += next_block->size;
    block_to_use->next = next_block->next;
    if (next_block->next) {
      next_block->next->prev = block_to_use;
    }

    // Liberamos metadata de next_block:
    if (next_block->name) free(next_block->name);
    free(next_block);
  }

  // 2) Si el bloque anterior existe y está libre, fusionamos hacia atrás:
  while (block_to_use->prev && block_to_use->prev->free) {
    Block* prev_block = block_to_use->prev;

    prev_block->size += block_to_use->size;
    prev_block->next = block_to_use->next;
    if (block_to_use->next) {
      block_to_use->next->prev = prev_block;
    }
    // Liberamos metadata de block_to_use (ya es libre y name == NULL):
    if (block_to_use->name) free(block_to_use->name);
    free(block_to_use);
    block_to_use = prev_block;
  }
}

/**************************************************************************************************
 * mm_free
 *
 *  Busca el bloque con name == name (y free == false). Si no está, error.
 *  Libera su metadata (free(name)), marca free = true y name = NULL. 
 *  Luego, (opcionalmente) hace memset en la región de datos con 0 para “borrar”.
 *  Finalmente invoca a mm_free_join(...) para unir bloques libres adyacentes.
 */
int mm_free(MemoryManagement* mm, const char* name) {
  Block* block_to_use = NULL;
  Block* current      = mm->start_block;

  while (current != NULL) {
    if (!current->free && current->name != NULL && strcmp(current->name, name) == 0) {
      block_to_use = current;
      break;
    }
    current = current->next;
  }
  if (block_to_use == NULL) {
    fprintf(stderr, "mm_free: No se encontró bloque con nombre %s.\n", name);
    return EXIT_FAILURE;
  }

  // 1) Liberamos la metadata (name):
  free(block_to_use->name);
  block_to_use->name = NULL;

  // 2) (Opcional) Borramos la región de datos para no “ver” el contenido
  //    Si queremos simular fuga, simplemente omitimos este memset. 
  //    Como la consigna pide “simular fuga” en uno de los ejemplos, 
  //    dejaremos esto comentado:
  // memset((char*)mm->memory_region + block_to_use->offset, 0, block_to_use->size);

  // 3) Marcamos el bloque como libre:
  block_to_use->free = true;

  // 4) Unimos con vecinos libres:
  mm_free_join(block_to_use);

  return EXIT_SUCCESS;
}

/**************************************************************************************************
 * mm_print
 *
 *  Imprime en stdout el estado actual de la lista de bloques. Por ejemplo:
 *    Memory Management:
 *    Block: 0, Offset: 0, Free, Size: 100
 *    Block: 1, Offset: 100, Name: A, Size: 200
 *    Block: 2, Offset: 300, Free, Size: 724
 * 
 *  (Índice de bloque, offset real en bytes, “Free” o “Name: XXX”, tamaño en bytes).
 */
void mm_print(const MemoryManagement* mm) {
  printf("Memory Management:\n");
  int    i       = 0;
  Block* current = mm->start_block;

  while (current != NULL) {
    printf("Block: %d, Offset: %zu, ", i, current->offset);
    if (current->free) {
      printf("Free, ");
    } else {
      printf("Name: %s, ", current->name);
    }
    printf("Size: %zu\n", current->size);

    i++;
    current = current->next;
  }
}

/**************************************************************************************************
 * mm_execute_command
 *
 *  Según command->type llama a la función adecuada:
 *    - CMD_ALLOC  -> mm_alloc(mm, command->name, command->size)
 *    - CMD_REALLOC -> mm_realloc(mm, command->name, command->size)
 *    - CMD_FREE   -> mm_free(mm, command->name)
 *    - CMD_PRINT  -> mm_print(mm)
 */
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
      fprintf(stderr,
              "mm_execute_command: Tipo de comando desconocido: %d.\n",
              command->type);
      return EXIT_FAILURE;
  }
}

/**************************************************************************************************
 * mm_start
 *
 *  1) Abre el archivo 'filename' para lectura.
 *  2) Lee cada línea con fgets(buffer,...).
 *  3) Si la línea está vacía o comienza con '#', la ignora.
 *  4) Invoca a parse_command(buffer, &command). Si falla, libera command.name y sale con ERROR.
 *  5) Invoca a mm_execute_command(mm, &command). Si falla, libera command.name y sale con ERROR.
 *  6) Si command.name != NULL, libera command.name antes de leer la siguiente línea.
 *  7) Al finalizar, cierra el archivo y devuelve EXIT_SUCCESS.
 */
int mm_start(MemoryManagement* mm, const char* filename) {
  FILE* file = fopen(filename, "r");
  if (file == NULL) {
    fprintf(stderr, "mm_start: No se pudo abrir el archivo: %s\n", filename);
    return EXIT_FAILURE;
  }

  char   buffer[256];
  while (fgets(buffer, sizeof(buffer), file)) {
    // Ignorar líneas vacías o comentarios
    if (buffer[0] == '\n' || buffer[0] == '#') {
      continue;
    }

    Command command;
    command.name = NULL;
    command.size = 0;

    if (parse_command(buffer, &command) != EXIT_SUCCESS) {
      if (command.name) free(command.name);
      fclose(file);
      fprintf(stderr, "mm_start: Error al parsear comando: %s", buffer);
      return EXIT_FAILURE;
    }

    if (mm_execute_command(mm, &command) != EXIT_SUCCESS) {
      if (command.name) free(command.name);
      fclose(file);
      fprintf(stderr, "mm_start: Error al ejecutar comando: %s", buffer);
      return EXIT_FAILURE;
    }

    if (command.name) {
      free(command.name);
      command.name = NULL;
    }
  }

  fclose(file);
  return EXIT_SUCCESS;
}
