#include <stdio.h>
#include "claves.h"
#include <mqueue.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_BUFFER   10000

mqd_t mqdes;

typedef struct Obj { // Definir la estructura del objeto que se envía por la cola
    int key;
    char value1[256];
    int N_value2;
    double V_value2[32];
    struct Coord value3;
    int operation;
} Obj;

int send_message(Obj msg) {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(Obj);
    attr.mq_curmsgs = 0;
    
    mqd_t mqdes = mq_open("/Cola4", O_WRONLY | O_CREAT, 0664, &attr);
    if (mqdes == (mqd_t)-1) {
        perror("Error al abrir la cola de mensajes");
        return -2;
    }

    if (mq_send(mqdes, (char *)&msg, sizeof(msg), 0) == -1) {
        perror("Error al enviar mensaje");
        mq_close(mqdes);
        return -2;
    }

    mq_close(mqdes);
    return 0;
}

int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    if (1 > N_value2 || N_value2 > 32){
        printf("El vector de elementos debe tener entre 1 y 32 elementos");
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

    send_message(obj);

    printf("Objeto enviado correctamente.\n");
    return 0;
}