#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>

struct Coord {
    int x, y;
};

typedef struct Obj {
    int key;
    char value1[256];  // Se usa un array en lugar de puntero para simplificar la serialización
    int N_value2;
    double V_value2[10];  // Se asume un máximo de 10 valores por simplicidad
    struct Coord value3;
} Obj;

int main() {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10; // Número máximo de mensajes en la cola
    attr.mq_msgsize = sizeof(Obj); // Tamaño máximo de cada mensaje
    attr.mq_curmsgs = 0;

    mqd_t mqdes = mq_open("/Cola2", O_RDONLY | O_CREAT, 0664, &attr);
    if (mqdes == (mqd_t)-1) {
        perror("Error al abrir la cola de mensajes");
        return -1;
    }

    Obj obj;
    ssize_t bytes_read = mq_receive(mqdes, (char *)&obj, sizeof(obj), NULL);
    if (bytes_read == -1) {
        perror("Error al recibir mensaje");
        return -1;
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

    // Cerrar la cola
    mq_close(mqdes);
    mq_unlink("/Cola1"); // Eliminar la cola al final

    return 0;
}