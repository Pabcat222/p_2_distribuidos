#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>   // Para sockaddr_in, inet_pton
#include <sys/socket.h>  // Para socket, connect, etc.
#include "claves.h"

#define MAX_BUFFER 256

// Ajustar según tu configuración
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 5000

// La misma estructura Obj que usabas antes.
// Si ya no vas a usar 'q_name', puedes quitarlo o ignorarlo.
typedef struct Obj {
    int key;
    char value1[MAX_BUFFER];
    int N_value2;
    double V_value2[32];
    struct Coord value3;
    int operation;
    char q_name[MAX_BUFFER]; 
} Obj;

/**
 * @brief Envía un objeto Obj al servidor (vía socket TCP) y recibe su respuesta en 'response'.
 * 
 * @param msg       Estructura con la petición (operation, key, etc.)
 * @param response  Estructura donde se almacenará la respuesta del servidor.
 * @return int  0 si no hay errores, -1 o -2 si algo falla.
 */
int send_message(Obj msg, Obj *response)
{
    // 1. Crear socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[CLIENTE] Error al crear socket");
        return -1;
    }

    // 2. Conectar al servidor en SERVER_IP:SERVER_PORT
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(SERVER_PORT);

    // Convierte la IP en formato texto a binario y la pone en sin_addr
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("[CLIENTE] Error con inet_pton");
        close(sock);
        return -2;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("[CLIENTE] Error al conectar al servidor");
        close(sock);
        return -2;
    }

    // 3. Enviar el objeto 'msg' al servidor
    ssize_t bytes_sent = send(sock, &msg, sizeof(msg), 0);
    if (bytes_sent < 0) {
        perror("[CLIENTE] Error al enviar datos al servidor");
        close(sock);
        return -2;
    }
    printf("[CLIENTE %d] Objeto enviado correctamente.\n", getpid());

    // 4. Recibir la respuesta en 'response'
    ssize_t bytes_recv = recv(sock, response, sizeof(*response), 0);
    if (bytes_recv < 0) {
        perror("[CLIENTE] Error al recibir respuesta del servidor");
        close(sock);
        return -2;
    } else if (bytes_recv == 0) {
        // Significa que el servidor cerró la conexión sin mandar nada
        printf("[CLIENTE %d] El servidor cerró la conexión inesperadamente.\n", getpid());
        close(sock);
        return -2;
    }

    // 5. Cerrar el socket
    close(sock);

    // 6. Mostrar el resultado devuelto
    printf("[CLIENTE %d] Servidor dice: %d\n", getpid(), response->operation);

    return 0;
}

/**
 * A partir de aquí, todas las funciones se mantienen prácticamente igual:
 * set_value, destroy, delete_key, exist, modify_value, get_value...
 * Únicamente llamamos a send_message(obj, &response) para hacer el intercambio.
 */

int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3)
{
    if (N_value2 < 1 || N_value2 > 32) {
        printf("[CLIENTE] Error: El vector debe tener entre 1 y 32 elementos\n");
        return -1;
    }

    Obj obj, response;
    memset(&obj, 0, sizeof(obj));
    memset(&response, 0, sizeof(response));

    obj.key = key;
    strncpy(obj.value1, value1, sizeof(obj.value1) - 1);
    obj.N_value2 = N_value2;
    memcpy(obj.V_value2, V_value2, N_value2 * sizeof(double));
    obj.value3 = value3;
    obj.operation = 1; // Indica "set_value"

    if (send_message(obj, &response) == 0) {
        // Aquí podrías interpretar response.operation como un código de retorno
        // Ej: if (response.operation == -1) => error
        return (response.operation == 0) ? 0 : -1;
    }
    return -1;
}

int destroy(void)
{
    Obj obj, response;
    memset(&obj, 0, sizeof(obj));
    memset(&response, 0, sizeof(response));

    obj.operation = 2; // "destroy"

    if (send_message(obj, &response) == 0) {
        return (response.operation == 0) ? 0 : -1;
    }
    return -1;
}

int delete_key(int key)
{
    Obj obj, response;
    memset(&obj, 0, sizeof(obj));
    memset(&response, 0, sizeof(response));

    obj.key = key;
    obj.operation = 3; // "delete_key"

    if (send_message(obj, &response) == 0) {
        // supondremos que response.operation = 0 si éxito, -1 si fallo
        return (response.operation == 0) ? 0 : -1;
    }
    return -1;
}

int exist(int key)
{
    Obj obj, response;
    memset(&obj, 0, sizeof(obj));
    memset(&response, 0, sizeof(response));

    obj.key = key;
    obj.operation = 4; // "exist"

    if (send_message(obj, &response) == 0) {
        // Este podría devolverte 1, 0 o -1 en response.operation
        return response.operation; 
    }
    return -1; // Error en comunicación
}

int modify_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3)
{
    if (N_value2 < 1 || N_value2 > 32) {
        printf("[CLIENTE] Error: El vector debe tener entre 1 y 32 elementos\n");
        return -1;
    }

    Obj obj, response;
    memset(&obj, 0, sizeof(obj));
    memset(&response, 0, sizeof(response));

    obj.key = key;
    strncpy(obj.value1, value1, sizeof(obj.value1) - 1);
    obj.N_value2 = N_value2;
    memcpy(obj.V_value2, V_value2, N_value2 * sizeof(double));
    obj.value3 = value3;
    obj.operation = 5; // "modify_value"

    if (send_message(obj, &response) == 0) {
        return (response.operation == 0) ? 0 : -1;
    }
    return -1;
}

int get_value(int key, char *value1, int *N_value2, double *V_value2, struct Coord *value3)
{
    Obj obj, response;
    memset(&obj, 0, sizeof(obj));
    memset(&response, 0, sizeof(response));

    obj.key = key;
    obj.operation = 6; // "get_value"

    if (send_message(obj, &response) != 0) {
        printf("[CLIENTE] Error al enviar el mensaje para get_value.\n");
        return -1;
    }

    // Ahora 'response' debería traer los datos
    // interpretados en tu servidor con operation=6
    // Supongamos que el servidor pone su resultado en response.value1, etc.
    strcpy(value1,         response.value1);
    *N_value2 =            response.N_value2;
    memcpy(V_value2,       response.V_value2, response.N_value2 * sizeof(double));
    value3->x =            response.value3.x;
    value3->y =            response.value3.y;

    if (response.operation == 0) {
        // Éxito
        return 0;
    } else {
        // Error
        return -1;
    }
}
