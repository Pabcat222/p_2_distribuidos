# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -std=c11 -fPIC
LDFLAGS = -lrt -pthread

# Target executables
TARGETS = app_cliente servidor-mq database

# Shared libraries
SHARED_LIBS = libproxy.so libclaves.so

# Object files
OBJS_CLIENTE = app_cliente.o libclaves.so
OBJS_SERVIDOR = servidor-mq.o libproxy.so
OBJS_DB = main.o claves.o

# Default target
all: $(TARGETS) $(SHARED_LIBS)

# Compile app_cliente
app_cliente: app_cliente.o libclaves.so
	$(CC) $(CFLAGS) -o $@ app_cliente.o -L. -lclaves $(LDFLAGS)

# Compile servidor-mq
servidor-mq: servidor-mq.o libproxy.so
	$(CC) $(CFLAGS) -o $@ servidor-mq.o -L. -lproxy $(LDFLAGS)

# Compile database
database: $(OBJS_DB)
	$(CC) $(CFLAGS) -o $@ $(OBJS_DB) -lsqlite3 $(LDFLAGS)

# Create shared libraries
libproxy.so: proxy-mq.o
	$(CC) -shared -o $@ $^

libclaves.so: claves.o
	$(CC) -shared -o $@ $^

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(TARGETS) $(OBJS_CLIENTE) $(OBJS_SERVIDOR) $(OBJS_DB) $(SHARED_LIBS)

# Phony targets
.PHONY: all clean
