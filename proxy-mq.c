#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>   // Para sockaddr_in, inet_pton
#include <sys/socket.h>  // Para socket, connect, etc.
#include "claves.h"

#define MAX_BUFFER 256

// Ajustar según tu configuración
/*#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 5000*/

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>   // inet_pton, sockaddr_in

int connect_to_server(void) {
    // 1. Obtener las variables de entorno
    char *ip_str = getenv("IP_TUPLAS");
    char *port_str = getenv("PORT_TUPLAS");

    if (!ip_str || !port_str) {
        fprintf(stderr, "Error: Las variables de entorno IP_TUPLAS y/o PORT_TUPLAS no están definidas.\n");
        fprintf(stderr, "Defínelas con: export IP_TUPLAS=<ip> y export PORT_TUPLAS=<puerto>\n");
        return -1;
    }

    // 2. Convertir el puerto a un número entero
    int port = atoi(port_str);
    if (port <= 0) {
        fprintf(stderr, "Error: El valor de PORT_TUPLAS no es válido: %s\n", port_str);
        return -1;
    }

    // 3. Crear el socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error al crear el socket");
        return -1;
    }

    // 4. Preparar la dirección del servidor
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convertir la IP a binario
    if (inet_pton(AF_INET, ip_str, &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Error al convertir la IP: %s\n", ip_str);
        close(sockfd);
        return -1;
    }

    // 5. Conectarse al servidor
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error al conectar al servidor");
        close(sockfd);
        return -1;
    }

    printf("Conectado al servidor %s:%d\n", ip_str, port);
    return sockfd;  // Devuelve el descriptor de socket para usar en el cliente
}





int send_message(Obj msg, Obj *response)
{
    // 1. Leer las variables de entorno
    char *ip_str   = getenv("IP_TUPLAS");
    char *port_str = getenv("PORT_TUPLAS");
    printf("IP: %s\n", ip_str);
    printf("IP: %s\n", port_str);

    if (!ip_str || !port_str) {
        fprintf(stderr, "[CLIENTE] Error: No están definidas las variables de entorno IP_TUPLAS y/o PORT_TUPLAS.\n");
        return -1;
    }

    // 2. Convertir el puerto a número entero
    int port = atoi(port_str);
    if (port <= 0) {
        fprintf(stderr, "[CLIENTE] Error: Valor de PORT_TUPLAS no válido: %s\n", port_str);
        return -1;
    }

    // 3. Crear el socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[CLIENTE] Error al crear socket");
        return -1;
    }

    // 4. Preparar la dirección del servidor
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(port);

    // Convertir la IP en binario
    if (inet_pton(AF_INET, ip_str, &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "[CLIENTE] Error al convertir la IP '%s'\n", ip_str);
        close(sock);
        return -1;
    }

    // 5. Conectarse al servidor
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("[CLIENTE] Error al conectar al servidor");
        close(sock);
        return -1;
    }

    printf("[CLIENTE %d] Conectado al servidor %s:%d\n", getpid(), ip_str, port);

    // 6. Enviar el objeto 'msg'
    ssize_t bytes_sent = send(sock, &msg, sizeof(msg), 0);
    if (bytes_sent < 0) {
        perror("[CLIENTE] Error al enviar datos al servidor");
        close(sock);
        return -1;
    }
    printf("[CLIENTE %d] Objeto enviado correctamente.\n", getpid());

    // 7. Recibir la respuesta
    ssize_t bytes_recv = recv(sock, response, sizeof(*response), 0);
    if (bytes_recv < 0) {
        perror("[CLIENTE] Error al recibir respuesta del servidor");
        close(sock);
        return -1;
    } else if (bytes_recv == 0) {
        // Significa que el servidor cerró la conexión sin mandar nada
        printf("[CLIENTE %d] El servidor cerró la conexión inesperadamente.\n", getpid());
        close(sock);
        return -1;
    }

    // 8. Cerrar el socket
    close(sock);

    // 9. Mostrar el resultado devuelto
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
