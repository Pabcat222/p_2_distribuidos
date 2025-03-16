#include <stdio.h>
#include "claves.h"
#include <mqueue.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>




#define MAX_BUFFER  256

mqd_t q_servidor;
mqd_t q_cliente;

/* 
   Definir la estructura del objeto que se envía por la cola, 
   esta estructura cuenta con los distintos valores que pueden 
   llegar a guardarse en la base de datos, la operación que 
   debe hacer el servidor en forma de entero, y el nombre de
   la cola del cliente
*/

typedef struct Obj { 
    int key;
    char value1[MAX_BUFFER];
    int N_value2;
    double V_value2[32];
    struct Coord value3;
    int operation;
    char q_name[MAX_BUFFER];
} Obj;


/*
    Función para enviar y recibir mensajes de las dos colas que se crean entre cliente y servidor.
    Tiene como argumentos msg (el oobjeto que se envía al servidor) y *response (donde se almacenarían 
    los valores proporcionados por el servidor en caso de ser necesario, dependiendo de la operación que se ejecute)
*/
int send_message(Obj msg, Obj *response) {

    // Características de las colas que se crean
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(Obj);
    attr.mq_curmsgs = 0;

    char queuename[MAX_BUFFER];
    sprintf(queuename, "/Cola-%d-5764-5879", getpid());
    strcpy(msg.q_name, queuename);
    // Crear colas
    q_cliente = mq_open(msg.q_name, O_CREAT|O_RDONLY, 0700, &attr);
    q_servidor = mq_open("/SERVIDOR-5764-5879", O_WRONLY);
    // Error al crear colas
    if (q_servidor == (mqd_t)-1) {
        perror("Error al abrir la cola de mensajes del servidor\n");
        return -2;
    }
    if (q_cliente == (mqd_t)-1) {
        perror("Error al abrir la cola de mensajes del cliente\n");
        return -2;
    }
    // Enviar mensaje con petición al servidor 
    if (mq_send(q_servidor, (char *)&msg, sizeof(Obj), 0) == -1) {
        perror("Error al enviar mensaje\n");
        mq_close(q_servidor);
        mq_close(q_cliente);
        mq_unlink(msg.q_name);
        return -2;
    }
    printf("[CLIENTE %d]: Objeto enviado correctamente.\n", getpid());

    // Recibir mensaje del servidor
    if (mq_receive(q_cliente, (char *)response, sizeof(Obj), 0) < 0){
		perror("Error al recibir desde el servidor");

        mq_close(q_servidor);
        mq_close(q_cliente);
        mq_unlink(msg.q_name);
		return -2;
    }
    printf("[CLIENTE %d]: Servidor dice: %d\n", getpid(), response->operation);
    
    // Cierre de colas y eliminación de la cola cliente
    mq_close(q_servidor);
    mq_close(q_cliente);
    mq_unlink(msg.q_name);
    return 0;
}


// Función set_value, para insertar datos en la base de datos
int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    if (1 > N_value2 || N_value2 > 32){
        printf("El vector de elementos debe tener entre 1 y 32 elementos\n");
        return -1;
    }
    Obj obj;
    Obj response;
    obj.key = key;
    strncpy(obj.value1, value1, sizeof(obj.value1) - 1);
    obj.value1[sizeof(obj.value1) - 1] = '\0';
    obj.N_value2 = (N_value2 > 10) ? 10 : N_value2;
    memcpy(obj.V_value2, V_value2, obj.N_value2 * sizeof(double));
    obj.value3 = value3;
    obj.operation = 1;

    if (send_message(obj, &response) == 0){
    return 0;
    }
    return -1; 
}

// Función destroy, para destruir la base de datos
int destroy(void){
    Obj obj;
    Obj response;
    obj.operation = 2;
    return send_message(obj, &response);
}

// Función delete_key, para borrar los datos asociados a una key dentro de la base de datos
int delete_key(int key){
    Obj obj;
    Obj response;
    obj.key = key;
    obj.operation = 3;
    return send_message(obj, &response);
}

// Función exist, para determinar si existen datos asociados a una determinada key dentro de la base de datos
int exist(int key){
    Obj obj;
    Obj response;
    obj.key = key;
    obj.operation = 4;
    return send_message(obj, &response);
}

// Función modify_value, para modificar los valores asociados a una key dentro de la base de datos
int modify_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3){
    if (1 > N_value2 || N_value2 > 32){
        printf("El vector de elementos debe tener entre 1 y 32 elementos\n");
        return -1;
    }
    Obj obj;
    Obj response;
    obj.key = key;
    strncpy(obj.value1, value1, sizeof(obj.value1) - 1);
    obj.value1[sizeof(obj.value1) - 1] = '\0';
    obj.N_value2 = (N_value2 > 10) ? 10 : N_value2;
    memcpy(obj.V_value2, V_value2, obj.N_value2 * sizeof(double));
    obj.value3 = value3;
    obj.operation = 5;
    return send_message(obj, &response);
}

// Función get_value, para obtener los datos asociados a una key dentro de la base de datos
int get_value(int key, char *value1, int *N_value2, double *V_value2, struct Coord *value3){
    Obj obj;
    obj.key = key;
    obj.operation = 6;
    Obj response;

    if (send_message(obj, &response) != 0) {
        printf("Error al enviar el mensaje para obtener el valor.\n");
        return -2;
    }

    
    strncpy(value1, response.value1, MAX_BUFFER);
    *N_value2 = response.N_value2;
    memcpy(V_value2, response.V_value2, response.N_value2 * sizeof(double));
    *value3 = response.value3;

    // Imprimir los valores recibidos
    printf("Valores recibidos del servidor:\n");
    printf("Value1: %s\n", value1);
    printf("N_value2: %d\n", *N_value2);
    printf("V_value2: ");
    for (int i = 0; i < *N_value2; i++) {
        printf("%.2f ", V_value2[i]);
    }
    printf("\nValue3: x=%d, y=%d\n", value3->x, value3->y);
  
    return 0;
}