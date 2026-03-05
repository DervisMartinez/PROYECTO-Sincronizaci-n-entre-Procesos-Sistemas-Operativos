# Makegile para compilar archvos en ele directorio actual
# NOmbre del ejecutable. Deseable en mayusculas.

TARGET = ECOFOLOW
# Bibliotecas incluidad, la biblioteca math.h es una muy comun
#LIBS = -lm
# Compilador utilazado, por ej, icc, pgcc, gcc
CC = gcc
# Banderas del compilador, por rj -DDEBUG - 02 - 03 -wall -g
CFLAGS = -Wall -Wextra -std=c99 -pthread -D_POSIX_C_SOURCE=200112L

# Palabras que usa el Makefile que podrian ser el nombre de un Programa
.PHONY: default all clean
# compilador por defecto

default: $(TARGET)
all: default
# incluye los archivos .o y ,c que estan en el directorio actual
OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
# incluye los archivos.h que estan en el directorio actual
HEADERS = $(wildcard *.h)
# compila automaticamente solo archivos fuente que se han modificado
# $< es el primer prerrequisito, generalmente en archivofuente
# $@ nombre del archivo que se esta generando, archivo objeto
%.o: %.c $(HEADERS)
		@$(CC) $(CFLAGS) -c $< -o $@
#preserva archivos intermedios
.PRECIOUS: $(TARGET) $(OBJECTS)
# enlaza objetos y crea el ejecutable
$(TARGET): $(OBJECTS)
	@$(CC) $(OBJECTS) $(LIBS) -o $@
	@echo "Programa compilado exitosamente"
# borra archivos .o
clean:
		-rm -f *.o core
# borra archivos .o y el ejecutable
cleanall: clean
	@	-rm -f $(TARGET)
