# TC03-MemoryManagement-OS
# Memory Management Simulator

Este proyecto implementa un simulador de gestión dinámica de memoria dentro de un bloque grande (por ejemplo, 1 MB) usando tres estrategias de asignación: **First-Fit**, **Best-Fit** y **Worst-Fit**. El programa lee un archivo de texto con comandos (`ALLOC`, `REALLOC`, `FREE`, `PRINT`) y administra internamente una lista enlazada de bloques libres/ocupados, simulando fragmentación, splits y joins, y rellenando cada bloque con el nombre de la variable.

---

## Tabla de contenidos

- [Descripción](#descripción)  
- [Requisitos](#requisitos)  
- [Instalación / Compilación](#instalación--compilación)  
- [Uso / Ejecución](#uso--ejecución)  
  - [Formato del archivo de comandos](#formato-del-archivo-de-comandos)  
  - [Ejemplo de ejecución](#ejemplo-de-ejecución)  
- [Estructura de archivos](#estructura-de-archivos)  
- [Pruebas adicionales](#pruebas-adicionales)  
- [Sanitización y debug](#sanitización-y-debug)  
- [Licencia](#licencia)

---

## Descripción

Este simulador recibe dos parámetros en tiempo de ejecución:

1. **Archivo de comandos** (un `.txt` con líneas como `ALLOC A 100`, `FREE B`, `REALLOC C 50`, `PRINT`, etc.).  
2. **Estrategia de asignación** (`first`, `best` o `worst`).

Internamente, el programa:

- Reserva un bloque contiguo grande (1 MB por defecto) con una sola llamada a `malloc`.  
- Mantiene una lista enlazada de estructuras `Block`, donde cada nodo tiene:
  - `free` (booleano)  
  - `name` (nombre de la variable cuando está ocupado)  
  - `size` (tamaño en bytes)  
  - `offset` (desplazamiento dentro del bloque grande)  
  - `prev` / `next` (enlaces de la lista)  

- Al ejecutar:
  - `ALLOC <nombre> <tamaño>`: busca un bloque libre de tamaño ≥ `<tamaño>` según la estrategia (First-Fit, Best-Fit o Worst-Fit).  
    - Si el bloque libre es mayor que `<tamaño>`, lo divide (split) y deja el remanente libre.  
    - Copia el nombre (`strdup`) en el nodo, marca `free=false` y **rellena la región de memoria** (offset … offset+size) con el primer carácter de `<nombre>`.  
  - `REALLOC <nombre> <nuevo_tamaño>`:  
    - Si el nuevo tamaño es mayor, intenta “crecer en sitio” (fusiones con bloques libres contiguos). Si no caben juntos, simula fuga: pide un nuevo bloque por separado, copiando el nombre y dejando el bloque viejo sin liberar.  
    - Si el nuevo tamaño es menor, hace “shrink”: reduce el bloque y crea un bloque libre contiguo con el remanente. En los dos casos, rellenado con el nombre.  
  - `FREE <nombre>`: busca el bloque con ese nombre, libera su metadata (`free(name)`), marca `free=true` y, opcionalmente, borra los datos. Luego une (join) con bloques libres adyacentes.  
  - `PRINT`: imprime por pantalla el estado de cada bloque (índice, offset, “Free” o “Name: XX”, tamaño).  

Con esto es posible visualizar cómo se reparte la memoria, observar fugas, fragmentación interna y externa, y comparar el comportamiento de las tres estrategias.

---

## Requisitos

- **Compilador GCC** (o equivalente compatible con C99).  
- **Make** (para compilar con el Makefile proporcionado).  
- Sistema operativo tipo Unix/Linux o cualquier terminal que soporte `gcc` y `make`.  

> **Nota**: También es posible compilar manualmente con `gcc` si no tienes `make`, pero se recomienda usar el Makefile para compilar todos los módulos adecuadamente.

---

## Instalación / Compilación

1. Clona (o descarga) el repositorio en tu máquina local.  
2. Entra en la carpeta raíz del proyecto (donde está el `Makefile`).  
3. Ejecuta:
   ```bash
   make
