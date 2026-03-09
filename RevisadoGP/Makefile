# Makefile para compilar archivos en el directorio actual
# Nombre del ejecutable (corregido: ECOFLOW en lugar de ECOFOLOW)
TARGET = ECOFLOW

# Bibliotecas necesarias (pthread para semáforos y threads, math opcional)
LIBS = -lpthread -lm

# Compilador utilizado
CC = gcc

# Banderas del compilador mejoradas
CFLAGS = -Wall -Wextra -std=c99 -pthread -D_POSIX_C_SOURCE=200112L -D_GNU_SOURCE

# Palabras que usa el Makefile que podrían ser el nombre de un programa
.PHONY: default all clean cleanall run

# Compilador por defecto
default: $(TARGET)
all: default

# Incluye los archivos .o y .c que están en el directorio actual
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)

# Incluye los archivos .h que están en el directorio actual
HEADERS = $(wildcard *.h)

# Compila automáticamente solo archivos fuente que se han modificado
# $< es el primer prerrequisito, generalmente el archivo fuente
# $@ nombre del archivo que se está generando, archivo objeto
%.o: %.c $(HEADERS)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compilado: $<"

# Preserva archivos intermedios
.PRECIOUS: $(TARGET) $(OBJECTS)

# Enlaza objetos y crea el ejecutable
$(TARGET): $(OBJECTS)
	@$(CC) $(OBJECTS) $(LIBS) -o $@
	@echo "===================================="
	@echo "✅ ECOFLOW compilado exitosamente"
	@echo "===================================="
	@echo "Ejecuta: ./$(TARGET)  o  make run"

# Borra archivos .o
clean:
	@rm -f *.o
	@echo "🧹 Archivos .o eliminados"

# Borra archivos .o y el ejecutable
cleanall: clean
	@rm -f $(TARGET)
	@echo "🧹 Ejecutable $(TARGET) eliminado"

# Ejecuta el programa (útil para pruebas rápidas)
run: $(TARGET)
	@echo "🚀 Ejecutando $(TARGET)..."
	@./$(TARGET)

# Muestra ayuda
help:
	@echo "Comandos disponibles:"
	@echo "  make        - Compila el programa"
	@echo "  make clean  - Elimina archivos objeto"
	@echo "  make cleanall - Elimina objetos y ejecutable"
	@echo "  make run    - Compila (si es necesario) y ejecuta"
	@echo "  make help   - Muestra esta ayuda"