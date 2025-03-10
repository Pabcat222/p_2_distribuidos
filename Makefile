# Compiler
CC = gcc

# Flags
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -lrt -pthread -lsqlite3

# Ejecutables
TARGETS = cliente servidor

all: $(TARGETS)

# Compilar el Cliente (app_cliente.c + proxy-mq.c)
cliente: app_cliente.o proxy-mq.o
	$(CC) $(CFLAGS) -o cliente app_cliente.o proxy-mq.o $(LDFLAGS)

# Compilar el Servidor (servidor-mq.c + claves.c)
servidor: servidor-mq.o claves.o
	$(CC) $(CFLAGS) -o servidor servidor-mq.o claves.o $(LDFLAGS)

# Compilar archivos fuente en objetos
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
