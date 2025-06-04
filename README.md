# TC03-MemoryManagement-OS

Este proyecto implementa un simulador de gestión dinámica de memoria dentro de un bloque contiguo de 1 MB. Permite probar tres estrategias de asignación: **First-Fit**, **Best-Fit** y **Worst-Fit**. El programa lee un archivo de texto con comandos (`ALLOC`, `REALLOC`, `FREE`, `PRINT`) y mantiene una lista enlazada de bloques libres/ocupados, simulando fragmentación, splits, joins y rellenando cada bloque con el nombre de la variable.

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
- [Sanitización y depuración](#sanitización-y-depuración)  
- [Licencia](#licencia)

---

## Descripción

El simulador funciona de la siguiente manera:

1. **Reserva un bloque contiguo de 1 MB** (mediante una única llamada a `malloc`).  
2. **Mantiene una lista doblemente enlazada de estructuras `Block`**, donde cada bloque almacena:
   - `free` (booleano): indica si está libre u ocupado.  
   - `name` (cadena): nombre de la variable que ocupa el bloque (si está ocupado).  
   - `size` (tamaño en bytes).  
   - `offset` (desplazamiento en bytes dentro del bloque de 1 MB).  
   - `prev` / `next`: punteros al bloque anterior y siguiente.  

3. Según los comandos del archivo de entrada:
   - `ALLOC <nombre> <tamaño>`: busca (First/Best/Worst-Fit) un bloque libre ≥ `<tamaño>`.  
     - Si el bloque encontrado es mayor, lo “divide” (split) y deja remanente libre.  
     - Asigna `<nombre>`, marca el bloque como ocupado y rellena esa región de memoria (desde `offset` hasta `offset+size`) con el primer carácter de `<nombre>`.  
   - `REALLOC <nombre> <nuevo_tamaño>`:  
     - Si `<nuevo_tamaño>` > tamaño actual, intenta crecer “en sitio” fusionando bloques contiguos. Si no cabe, simula fuga: pide un bloque nuevo, copia el nombre, deja el viejo sin liberar.  
     - Si `<nuevo_tamaño>` < tamaño actual, reduce el bloque (shrink) y crea un bloque libre con el remanente. En ambos casos, vuelve a rellenar con el nombre.  
   - `FREE <nombre>`: busca el bloque ocupado con ese nombre, libera su metadata (`free(name)`), marca el bloque como libre y une (join) con bloques libres contiguos.  
   - `PRINT`: imprime en consola todos los bloques (índice, offset, “Free” o “Name: XX”, tamaño).

Así se puede visualizar la fragmentación interna/externa, detectar fugas y comparar el comportamiento de las tres estrategias.

---

## Requisitos

- **Compilador GCC** (o cualquier compilador compatible con C99).  
- **Make** (opcional, pero recomendado para usar el Makefile).  
- Sistema operativo Unix-like o Windows con entorno que soporte `gcc` y `make` (WSL, MSYS2, Git Bash, etc.).

> Si no se dispone de `make`, se puede compilar manualmente con `gcc` (ver [Sección “Uso manual de GCC”](#uso--ejecución)).

---

## Instalación / Compilación

1. Clona (o descarga) este repositorio.  
2. Entra en la carpeta raíz del proyecto (donde está el `Makefile`).  
3. Ejecuta:
   ```bash
   make


Manualmente o desde la raiz

mkdir -p build bin

# Compilar cada módulo
gcc -I./src -Werror -Wall -Wextra -c src/parser.c            -o build/parser.o
gcc -I./src -Werror -Wall -Wextra -c src/memory_management.c -o build/memory_management.o
gcc -I./src -Werror -Wall -Wextra -c src/main.c              -o build/main.o

# Enlazar para generar el ejecutable
gcc build/parser.o build/memory_management.o build/main.o -o bin/memory_management


Archivo de entrada con comandos
# Línea de comentario (se ignora)
ALLOC <nombre> <tamaño>    # Reserva <tamaño> bytes para <nombre>
REALLOC <nombre> <tamaño>  # Cambia el tamaño del bloque <nombre>
FREE <nombre>              # Libera el bloque asignado a <nombre>
PRINT                      # Muestra el estado actual de todos los bloques

Ejecución con make

# First-Fit
make run ARGS="data/1.txt first"

# Best-Fit
make run ARGS="data/1.txt best"

# Worst-Fit
make run ARGS="data/1.txt worst"

en el make predeterminado 
ARGS = data/1.txt worst

run: $(TARGET)
	$(TARGET) $(ARGS)


Ejecucion manual con gcc

# En Linux/Mac:
./bin/memory_management data/1.txt first

# En Windows (CMD o PowerShell):
.\bin\memory_management.exe .\data\1.txt first


para probar

./bin/memory_management data/2.txt first
./bin/memory_management data/3.txt best
 ejemplos
 ./bin/memory_management data/1.txt best


 Memory Management:
Block: 0, Offset: 0, Name: C, Size: 50
Block: 1, Offset: 50, Free, Size: 50
Block: 2, Offset: 100, Name: B, Size: 200
Block: 3, Offset: 300, Free, Size: 1048276

Memory Management:
Block: 0, Offset: 0, Name: C, Size: 50
Block: 1, Offset: 50, Free, Size: 50
Block: 2, Offset: 100, Name: B, Size: 200
Block: 3, Offset: 300, Name: D, Size: 300
Block: 4, Offset: 600, Free, Size: 1047976

