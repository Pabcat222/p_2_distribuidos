# Compiler
CC = gcc

# Flags de compilación
CFLAGS = -Wall -Wextra -std=c11 -fPIC
LDFLAGS = -lrt -pthread -lsqlite3

# Nombre de la biblioteca dinámica
LIBRARY = libclaves.so

# Ejecutables
TARGETS = cliente cliente1 cliente2 cliente3 cliente4 cliente5 cliente6 servidor

all: $(LIBRARY) $(TARGETS)

# Crear la biblioteca compartida (libclaves.so) solo con el código del cliente
$(LIBRARY): proxy-mq.o
	$(CC) -shared -o $(LIBRARY) proxy-mq.o $(LDFLAGS)

# Compilar el Servidor (usa claves.c, NO usa libclaves.so)
servidor: servidor-mq.o claves.o
	$(CC) $(CFLAGS) -o servidor servidor-mq.o claves.o $(LDFLAGS)

# Compilar los Clientes usando la biblioteca compartida
cliente: app_cliente.o
	$(CC) $(CFLAGS) -o cliente app_cliente.o -L. -lclaves $(LDFLAGS)

cliente1: cliente1.o
	$(CC) $(CFLAGS) -o cliente1 cliente1.o -L. -lclaves $(LDFLAGS)

cliente2: cliente2.o
	$(CC) $(CFLAGS) -o cliente2 cliente2.o -L. -lclaves $(LDFLAGS)

cliente3: cliente3.o
	$(CC) $(CFLAGS) -o cliente3 cliente3.o -L. -lclaves $(LDFLAGS)

cliente4: cliente4.o
	$(CC) $(CFLAGS) -o cliente4 cliente4.o -L. -lclaves $(LDFLAGS)

cliente5: cliente5.o
	$(CC) $(CFLAGS) -o cliente5 cliente5.o -L. -lclaves $(LDFLAGS)

cliente6: cliente6.o
	$(CC) $(CFLAGS) -o cliente6 cliente6.o -L. -lclaves $(LDFLAGS)


# Compilar archivos fuente en objetos
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -f $(TARGETS) *.o $(LIBRARY)

.PHONY: all clean
