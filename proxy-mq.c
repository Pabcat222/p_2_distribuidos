#include <stdio.h>
#include "claves.h"
#include <mqueue.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>




#define MAX_BUFFER   10000

mqd_t q_servidor;
mqd_t q_cliente;

typedef struct Obj { // Definir la estructura del objeto que se envía por la cola
    int key;
    char value1[256];
    int N_value2;
    double V_value2[32];
    struct Coord value3;
    int operation;
    char q_name[256];
} Obj;

int send_message(Obj msg, Obj *response) {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(Obj);
    attr.mq_curmsgs = 0;

    char queuename[256];
    sprintf(queuename, "/Cola-%d-5764-5879", getpid());
    strcpy(msg.q_name, queuename);
    printf("Este es el nombre de la cola del cliente: %s\n", msg.q_name);
    //CREAR COLAS
    q_cliente = mq_open(msg.q_name, O_CREAT|O_RDONLY, 0700, &attr);
    q_servidor = mq_open("/SERVIDOR-5764-5879", O_WRONLY);
    //ERROR AL CREAR COLAS
    if (q_servidor == (mqd_t)-1) {
        perror("Error al abrir la cola de mensajes del servidor\n");
        return -2;
    }
    if (q_cliente == (mqd_t)-1) {
        perror("Error al abrir la cola de mensajes del cliente\n");
        return -2;
    }

    if (mq_send(q_servidor, (char *)&msg, sizeof(Obj), 0) == -1) {
        perror("Error al enviar mensaje\n");
        mq_close(q_servidor);
        mq_close(q_cliente);
        mq_unlink(msg.q_name);
        return -2;
    }
    printf("Objeto enviado correctamente.\n");

    if (mq_receive(q_cliente, (char *)response, sizeof(Obj), 0) < 0){
		perror("Error al recibir desde el servidor");

        mq_close(q_servidor);
        mq_close(q_cliente);
        mq_unlink(msg.q_name);
		return -2;
    }
    printf("Servidor dice: %d\n", msg.operation);
    
    printf("Este es el nombre de la cola del cliente 2: %s \n", msg.q_name);
    mq_close(q_servidor);
    mq_close(q_cliente);
    mq_unlink(msg.q_name);
    return 0;
}

int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    if (1 > N_value2 || N_value2 > 32){
        printf("El vector de elementos debe tener entre 1 y 32 elementos\n");
        return -1;
    }
    Obj obj;
    
    obj.key = key;
    strncpy(obj.value1, value1, sizeof(obj.value1) - 1);
    obj.value1[sizeof(obj.value1) - 1] = '\0';
    obj.N_value2 = (N_value2 > 10) ? 10 : N_value2;
    memcpy(obj.V_value2, V_value2, obj.N_value2 * sizeof(double));
    obj.value3 = value3;
    obj.operation = 1;

    if (send_message(obj, NULL) == 0){
    return 0;
    }
    return -1; 
}

int destroy(void){
    Obj obj;
    obj.operation = 2;
    return send_message(obj, NULL);
}

int delete_key(int key){
    Obj obj;
    obj.key = key;
    obj.operation = 3;
    return send_message(obj, NULL);
}

int exist(int key){
    Obj obj;
    obj.key = key;
    obj.operation = 4;
    return send_message(obj, NULL);
}

int modify_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3){
    if (1 > N_value2 || N_value2 > 32){
        printf("El vector de elementos debe tener entre 1 y 32 elementos\n");
        return -1;
    }
    Obj obj;
    
    obj.key = key;
    strncpy(obj.value1, value1, sizeof(obj.value1) - 1);
    obj.value1[sizeof(obj.value1) - 1] = '\0';
    obj.N_value2 = (N_value2 > 10) ? 10 : N_value2;
    memcpy(obj.V_value2, V_value2, obj.N_value2 * sizeof(double));
    obj.value3 = value3;
    obj.operation = 5;
    return send_message(obj, NULL);
}

int get_value(int key, char *value1, int *N_value2, double *V_value2, struct Coord *value3){
    Obj obj;
    obj.key = key;
    obj.operation = 6;
    Obj response;

    if (send_message(obj, &response) != 0) {
        printf("Error al enviar el mensaje para obtener el valor.\n");
        return -1;
    }

    printf("Ha llegao\n");
    
    strncpy(value1, response.value1, 256);
    *N_value2 = response.N_value2;
    memcpy(V_value2, response.V_value2, response.N_value2 * sizeof(double));
    *value3 = response.value3;
    return 0;
}