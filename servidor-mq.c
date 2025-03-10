#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <pthread.h>
#include "claves.h"

pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int key;
    char value1[256];
    int N_value2;
    double V_value2[32];
    struct Coord value3;
    int operation;
} Obj;   

int main() {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(Obj);
    attr.mq_curmsgs = 0;
    
    mqd_t mqdes = mq_open("/Cola4", O_RDONLY | O_CREAT, 0664, &attr);
    if (mqdes == (mqd_t)-1) {
        perror("Error al abrir la cola de mensajes");
        return -2;
    }

    printf("Servidor en espera de mensajes...\n");

    while (1) {
        Obj obj;  
        ssize_t bytes_read = mq_receive(mqdes, (char *)&obj, sizeof(obj), NULL);
        if (bytes_read == -1) {
            perror("Error al recibir mensaje");
            continue;
        }

        // Imprimir los valores recibidos
        printf("Mensaje recibido:\n");
        printf("  key: %d\n", obj.key);
        printf("  value1: %s\n", obj.value1);
        printf("  N_value2: %d\n", obj.N_value2);
        printf("  V_value2: ");
        for (int i = 0; i < obj.N_value2; i++) {
            printf("%.2f ", obj.V_value2[i]);
        }
        printf("\n  value3: (%d, %d)\n", obj.value3.x, obj.value3.y);
        printf("  operacion: %d\n", obj.operation);

        pthread_mutex_lock(&db_mutex);

        // Insertar en la base de datos
        
        if (set_value(obj.key, obj.value1, obj.N_value2, obj.V_value2, obj.value3) == -1) {
            printf("Error: No se pudo guardar el mensaje en la base de datos.\n");
        } else {
            printf("Mensaje guardado correctamente en la base de datos.\n");
        }

        pthread_mutex_unlock(&db_mutex);
    }
    mq_close(mqdes);
    mq_unlink("/Cola4");

    return 0;
}