# Compiler
CC = gcc

# Flags
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -lrt -pthread -lsqlite3

# Ejecutables
TARGETS = cliente cliente1 cliente2 cliente3 servidor

all: $(TARGETS)

# Compilar el Cliente (app_cliente.c + proxy-mq.c)
cliente: app_cliente.o proxy-mq.o
	$(CC) $(CFLAGS) -o cliente app_cliente.o proxy-mq.o $(LDFLAGS)

# Compilar Cliente 1 (cliente1.c + proxy-mq.c)
cliente1: cliente1.o proxy-mq.o
	$(CC) $(CFLAGS) -o cliente1 cliente1.o proxy-mq.o $(LDFLAGS)

# Compilar Cliente 2 (cliente2.c + proxy-mq.c)
cliente2: cliente2.o proxy-mq.o
	$(CC) $(CFLAGS) -o cliente2 cliente2.o proxy-mq.o $(LDFLAGS)

# Compilar Cliente 3 (cliente3.c + proxy-mq.c)
cliente3: cliente3.o proxy-mq.o
	$(CC) $(CFLAGS) -o cliente3 cliente3.o proxy-mq.o $(LDFLAGS)

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