#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <netinet/in.h>   // sockaddr_in
#include <sys/socket.h>   // socket, bind, listen, accept
#include <arpa/inet.h>    // inet_addr (si lo necesitas)
#include "claves.h"

// Puerto donde escuchará el servidor:
#define SERVER_PORT 5000

// Estructura que usabas en mqueues
typedef struct {
    int key;
    char value1[256];
    int N_value2;
    double V_value2[32];
    struct Coord value3;
    int operation;
    char q_name[256]; // Podrías ignorar 'q_name' si ya no lo usas
} Obj;

// Para sincronizar la base de datos en hilos
pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

static int server_socket_fd;
static volatile int stop_server = 0;  // Para parar el servidor con Ctrl+C (opcional)


// ---------------------------------------------------------------------------
// Función que maneja la señal Ctrl+C para parar el servidor ordenadamente
// ---------------------------------------------------------------------------
void cerrar_servidor(int sig)
{
    (void) sig; // no usado
    printf("\n[SERVIDOR] Recibida señal de cierre. Cerrando...\n");
    stop_server = 1;
    // Cierra el socket de escucha para desbloquear el accept
    close(server_socket_fd);
}

// ---------------------------------------------------------------------------
// Función que atiende la petición de un cliente en un hilo
// Recibe el socket (ya aceptado) y procesa el objeto.
// ---------------------------------------------------------------------------
void *tratar_conexion(void *arg)
{
    int client_fd = *(int*)arg;
    free(arg); // Liberamos la memoria reservada para el client_fd
    Obj obj;
    Obj respuesta;
    memset(&obj,       0, sizeof(obj));
    memset(&respuesta, 0, sizeof(respuesta));

    // Recibir la estructura Obj completa (un único mensaje)
    ssize_t r = recv(client_fd, &obj, sizeof(obj), 0);
    if (r <= 0) {
        printf("[SERVIDOR] Error o conexión cerrada antes de recibir datos.\n");
        close(client_fd);
        return NULL;
    }

    // Vamos a procesar la operación (igual que hacías en tratarMensaje).
    // Aseguramos acceso atómico a la base de datos con el mutex db_mutex.
    pthread_mutex_lock(&db_mutex);

    int res = 0; // valor a devolver al cliente (puede ser -1, 0, 1, etc.)
    switch (obj.operation) {
        case 1: // set_value
            printf("[SERVIDOR] Se recibió 'set_value' con key=%d\n", obj.key);
            if (set_value(obj.key, obj.value1, obj.N_value2, obj.V_value2, obj.value3) == -1) {
                printf("[SERVIDOR] Error: No se pudo guardar en la BD.\n");
                res = -1;
            } else {
                printf("[SERVIDOR] Insertado correctamente.\n");
                res = 0;
            }
            break;

        case 2: // destroy
            printf("[SERVIDOR] Se recibió 'destroy'\n");
            if (destroy() == -1) {
                printf("[SERVIDOR] Error al eliminar BD.\n");
                res = -1;
            } else {
                res = 0;
            }
            break;

        case 3: // delete_key
            printf("[SERVIDOR] Se recibió 'delete_key'\n");
            res = delete_key(obj.key);
            break;

        case 4: // exist
            printf("[SERVIDOR] Se recibió 'exist'\n");
            res = exist(obj.key); 
            // valores: -1 error, 0 no existe, 1 existe
            break;

        case 5: // modify_value
            printf("[SERVIDOR] Se recibió 'modify_value'\n");
            if (modify_value(obj.key, obj.value1, obj.N_value2, obj.V_value2, obj.value3) == -1) {
                printf("[SERVIDOR] Error al modificar.\n");
                res = -1;
            } else {
                res = 0;
            }
            break;

        case 6: // get_value
            printf("[SERVIDOR] Se recibió 'get_value'\n");
            {
                // Lo guardamos en variables temporales:
                char value1[256];
                int  N_value2;
                double V_value2[32];
                struct Coord value3;

                if (get_value(obj.key, value1, &N_value2, V_value2, &value3) == 0) {
                    // Montamos la respuesta con esos datos
                    strcpy(respuesta.value1, value1);
                    respuesta.N_value2 = N_value2;
                    memcpy(respuesta.V_value2, V_value2, N_value2 * sizeof(double));
                    respuesta.value3 = value3;
                    res = 0;
                } else {
                    printf("[SERVIDOR] Error al obtener valores.\n");
                    res = -1;
                }
            }
            break;

        default:
            printf("[SERVIDOR] Operación no reconocida: %d\n", obj.operation);
            res = -1;
            break;
    }

    // Liberamos el mutex
    pthread_mutex_unlock(&db_mutex);

    // Ahora enviamos la respuesta. Tú decidías entre enviar un simple 'int' o toda la struct.
    // Supongamos que en 'respuesta.operation' guardamos el resultado (res).
    // Y si es get_value, además devolvemos los datos en 'respuesta'.
    respuesta.operation = res;

    // Enviar al cliente
    ssize_t s = send(client_fd, &respuesta, sizeof(respuesta), 0);
    if (s == -1) {
        printf("[SERVIDOR] Error al enviar la respuesta.\n");
    }

    close(client_fd); // Cerramos la conexión con el cliente
    return NULL;
}

// ---------------------------------------------------------------------------
// Función principal (main) que arranca el servidor socket
// ---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    if (argc<2){
        fprintf(stderr, "Uso: %s <PUERTO>\n", argv[0]);
        return 1;
    }

    int port = atoi (argv[1]);
    // Instalar la señal de Ctrl+C para poder cerrar ordenadamente
    signal(SIGINT, cerrar_servidor);

    // Crear el socket
    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd < 0) {
        perror("[SERVIDOR] Error al crear socket");
        return 1;
    }

    // Estructura para bind
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // o inet_addr("0.0.0.0")
    server_addr.sin_port        = htons(port);

    // Vincular el socket a la IP/puerto
    if (bind(server_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[SERVIDOR] Error en bind");
        close(server_socket_fd);
        return 1;
    }

    // Ponemos el socket en modo 'escucha'
    if (listen(server_socket_fd, 5) < 0) {
        perror("[SERVIDOR] Error en listen");
        close(server_socket_fd);
        return 1;
    }

    printf("[SERVIDOR] Escuchando en el puerto %d...\n", port);

    while (!stop_server) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        // Aceptamos la conexión de un cliente
        int client_fd = accept(server_socket_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            // Si el accept falla debido al cierre del servidor (Ctrl+C),
            // simplemente salimos del bucle.
            if (stop_server) break; 
            perror("[SERVIDOR] Error en accept");
            continue;
        }

        // Creamos un hilo que atienda al cliente
        pthread_t tid;
        // Reservamos memoria para pasar el client_fd al hilo
        int *arg = malloc(sizeof(int));
        if (!arg) {
            fprintf(stderr, "[SERVIDOR] Error al asignar memoria\n");
            close(client_fd);
            continue;
        }
        *arg = client_fd;

        if (pthread_create(&tid, NULL, tratar_conexion, arg) != 0) {
            perror("[SERVIDOR] Error al crear hilo");
            close(client_fd);
            free(arg);
            continue;
        }

        // Podemos hacer que el hilo sea "detached" para que se libere solo
        pthread_detach(tid);
    }

    close(server_socket_fd);
    printf("[SERVIDOR] Finalizado.\n");
    return 0;
}