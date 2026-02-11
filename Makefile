# Define required macros here
SHELL = /bin/sh

# Directorios
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
PMLIB_DIR = ${HOME}/pmlib

# Nombre del ejecutable
EXE = linear

# Lista de objetos (ahora divididos por módulos)
OBJS = $(OBJ_DIR)/main.o \
       $(OBJ_DIR)/utils.o \
       $(OBJ_DIR)/filter_kernels.o \
       $(OBJ_DIR)/filter_bank.o

# Compilador y Flags
CC = gcc
CFLAGS = -w -O3 -fopenmp -march=native
LDFLAGS = -fopenmp -lm

# Inclusiones (añadimos nuestro INC_DIR y el de pmlib)
INCLUDES = -I$(INC_DIR) -I${PMLIB_DIR}/client
LIBS = ${PMLIB_DIR}/client/libpmlib.so

# Regla principal
$(EXE): $(OBJ_DIR) $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBS) $(INCLUDES)

# Crear el directorio de objetos si no existe
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Regla genérica para compilar los .c en el directorio src
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Limpieza
clean:
	-rm -rf $(OBJ_DIR) core *.core $(EXE)

.PHONY: clean
