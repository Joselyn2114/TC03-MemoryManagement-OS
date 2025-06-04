//

#ifndef MEMORY_MANAGEMENT_H
#define MEMORY_MANAGEMENT_H

#include <stdbool.h>
#include <stddef.h>

#include "command.h"
#include "strategy.h"

/**
 * Cada bloque de la lista representa:
 *  - free == true  ⇾ un trozo libre de 'size' bytes a partir de 'offset' bytes desde memory_region.
 *  - free == false ⇾ un trozo ocupado con nombre 'name', 'size' bytes, en 'offset' bytes desde memory_region.
 */

typedef struct Block {
  bool free;
  char* name;
  size_t size;
  size_t offset;     // desplazamiento (en bytes) desde memory_region
  struct Block* next;
  struct Block* prev;
} Block;

/**
 * Estructura principal de manejo de memoria:
 *  - strategy: enum { FIRST, BEST, WORST }
 *  - total_size: tamaño total del bloque grande
 *  - memory_region: puntero al bloque grande (void*) que pedimos con malloc()
 *  - start_block: head de la lista doblemente enlazada de Block
 */

typedef struct {
  StrategyType strategy;
  size_t size;
  Block* start_block;
} MemoryManagement;

/**
 * mm_init:
 *  - strategy: cuál algoritmo usar (FIRST, BEST, WORST)
 *  - size: tamaño (en bytes) para pedir al SO
 * 
 *  Reserva memory_region = malloc(size) y crea el bloque inicial libre:
 *    offset = 0, size = total_size, free = true, name = NULL.
 */
int mm_init(MemoryManagement* mm, StrategyType strategy, size_t size);

/**
 * mm_destroy:
 *  - Libera todos los bloques de la lista (metadata) y libera memory_region.
 */
void mm_destroy(MemoryManagement* mm);

/**
 * mm_alloc:
 *  - name: nombre de la variable (por ejemplo, "A", "B"…)
 *  - size: cuántos bytes queremos reservar
 * 
 *  Encuentra un bloque adecuado según strategy, hace split si es necesario,
 *  guarda name en Block, marca free = false, y sobre la región de datos 
 *  correspondiente hace memset con el primer carácter de name para “llenar”.
 */
int mm_alloc(MemoryManagement* mm, const char* name, size_t size);

/**
 * mm_alloc_split:
 *  - block_to_use: bloque libre con tamaño >= size
 *  - size: tamaño deseado para el bloque ocupado
 * 
 *  Divide block_to_use en dos, si el remanente es > sizeof(Block). 
 *  El bloque original queda con size = size, 
 *  y el nuevo bloque libre se crea con el resto (offset ajustado).
 */
int mm_alloc_split(Block* block_to_use, size_t size);

/**
 * mm_realloc:
 *  - name: nombre de variable (existe un bloque ocupado con este name)
 *  - size: nuevo tamaño deseado.
 *  
 *  Si size == size_actual, no hace nada.
 *  Si size > size_actual, intenta crecer en sitio: 
 *    -> si el siguiente bloque es libre y la suma de ambos >= size, los fusiona (join) y ajusta/ hace split final.
 *    -> si no se puede, hace mm_alloc(...) en otro trozo, copiando el nombre, y deja el bloque viejo libre (simula fuga).
 *  Si size < size_actual, hace mm_realloc_shrink(...) para cortar y crear un bloque libre con el remanente.
 */
int mm_realloc(MemoryManagement* mm, const char* name, size_t size);

/**
 * mm_realloc_grow:
 *  - mm: estructura completa
 *  - block_to_use: bloque ocupado a expandir
 *  - size: nuevo tamaño
 * 
 *  Intenta fusionar con el siguiente bloque si está libre. 
 *  Si la unión >= size, hace join y split del remanente.
 *  Si no se puede (o next no libre), devuelve EXIT_FAILURE para que el llamador lo trate (fuga).
 */
int mm_realloc_grow(MemoryManagement* mm, Block* block_to_use, size_t size);

/**
 * mm_realloc_shrink:
 *  - block_to_use: bloque ocupado
 *  - size: tamaño menor al actual
 * 
 *  Corta block_to_use a 'size' bytes, crea un nuevo bloque libre con el remanente.
 *  Rellena (memset) la parte ocupada con el nombre viejo.
 */
int mm_realloc_shrink(Block* block_to_use, size_t size);

/**
 * mm_free:
 *  - name: nombre de la variable a liberar
 * 
 *  Busca el bloque con ese name. Si no lo encuentra, devuelve error.
 *  Libera su metadata->name, rellena con 0 (opcional), marca free = true, 
 *  y hace mm_free_join(...) para unir con vecinos libres.
 */
int mm_free(MemoryManagement* mm, const char* name);

/**
 * mm_free_join:
 *  - block_to_use: bloque recién liberado
 *  
 *  Si el siguiente bloque está libre, los une en un solo bloque. 
 *  Repite mientras haya bloques libres contiguos adelante o atrás.
 */
void mm_free_join(Block* block_to_use);

/**
 * mm_print:
 *  - mm: estado actual
 * 
 *  Imprime línea por línea todos los bloques (libres u ocupados), mostrando:
 *    índice, offset, estado (Free o Name), tamaño.
 */
void mm_print(const MemoryManagement* mm);

/**
 * mm_start:
 *  - mm: estructura completa
 *  - filename: ruta archivo de comandos
 * 
 *  Abre el archivo, lee línea a línea con fgets,
 *  ignora líneas vacías o que empiezan con '#', llama a parse_command(...)
 *  y luego a mm_execute_command(...). 
 *  Maneja error si parse o execute fallan.
 */
int mm_start(MemoryManagement* mm, const char* filename);

/**
 * mm_execute_command:
 *  - mm: estado actual
 *  - command: puntero a estructura Command (type, name, size)
 * 
 *  Según command->type invoca a mm_alloc, mm_realloc, mm_free o mm_print.
 */
int mm_execute_command(MemoryManagement* mm, const Command* command);

/**
 * mm_find_block:
 *  - mm: estado actual
 *  - requested_size: cuántos bytes queremos
 * 
 *  Despacha a la función concreta segun mm->strategy:
 *    - FIRST → mm_find_block_first_fit
 *    - BEST  → mm_find_block_best_fit
 *    - WORST → mm_find_block_worst_fit
 */
Block* mm_find_block(MemoryManagement* mm, size_t requested_size);

/**
 * Algoritmos de búsqueda de bloque:
 *  - mm_find_block_first_fit: primer bloque libre con size >= requested_size.
 *  - mm_find_block_best_fit: bloque libre con size >= requested_size y size mínimo.
 *  - mm_find_block_worst_fit: bloque libre con size >= requested_size y size máximo.
 */
Block* mm_find_block_first_fit(MemoryManagement* mm, size_t requested_size);
Block* mm_find_block_best_fit(MemoryManagement* mm, size_t requested_size);
Block* mm_find_block_worst_fit(MemoryManagement* mm, size_t requested_size);

#endif  // MEMORY_MANAGEMENT_H