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
$(LIBRARY): proxy-sock.o
	$(CC) -shared -o $(LIBRARY) proxy-sock.o $(LDFLAGS)

# Compilar el Servidor (usa claves.c, NO usa libclaves.so)
servidor: servidor-sock.o claves.o
	$(CC) $(CFLAGS) -o servidor servidor-sock.o claves.o $(LDFLAGS)

# Compilar los Clientes usando la biblioteca compartida
cliente: app-cliente.o
	$(CC) $(CFLAGS) -o cliente app-cliente.o -L. -lclaves $(LDFLAGS)

cliente1: app-cliente-1.o
	$(CC) $(CFLAGS) -o cliente1 app-cliente-1.o -L. -lclaves $(LDFLAGS)

cliente2: app-cliente-2.o
	$(CC) $(CFLAGS) -o cliente2 app-cliente-2.o -L. -lclaves $(LDFLAGS)

cliente3: app-cliente-3.o
	$(CC) $(CFLAGS) -o cliente3 app-cliente-3.o -L. -lclaves $(LDFLAGS)

cliente4: app-cliente-4.o
	$(CC) $(CFLAGS) -o cliente4 app-cliente-4.o -L. -lclaves $(LDFLAGS)

cliente5: app-cliente-5.o
	$(CC) $(CFLAGS) -o cliente5 app-cliente-5.o -L. -lclaves $(LDFLAGS)

cliente6: app-cliente-6.o
	$(CC) $(CFLAGS) -o cliente6 app-cliente-6.o -L. -lclaves $(LDFLAGS)


# Compilar archivos fuente en objetos
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -f $(TARGETS) *.o $(LIBRARY)

.PHONY: all clean
