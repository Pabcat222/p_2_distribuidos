#include <stdio.h>
#include "claves.h"
#include <mqueue.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_BUFFER   10000

mqd_t mqdes;

typedef struct Obj {
    int key;
    char value1[256];
    int N_value2;
    double V_value2[10];
    struct Coord value3;
} Obj;

int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    mqd_t mqdes = mq_open("/Cola2", O_WRONLY);
    if (mqdes == (mqd_t)-1) {
        perror("Error al abrir la cola de mensajes");
        return -1;
    }

    Obj obj;
    obj.key = key;
    strncpy(obj.value1, value1, sizeof(obj.value1) - 1);
    obj.value1[sizeof(obj.value1) - 1] = '\0';
    obj.N_value2 = (N_value2 > 10) ? 10 : N_value2;
    memcpy(obj.V_value2, V_value2, obj.N_value2 * sizeof(double));
    obj.value3 = value3;

    if (mq_send(mqdes, (char *)&obj, sizeof(obj), 0) == -1) {
        perror("Error al enviar el mensaje");
        return -1;
    }

    printf("Objeto enviado correctamente.\n");
    mq_close(mqdes);
    return 0;
}

int main() {
    struct Coord coord = {5, 10};
    double valores[] = {10.5, 20.3, 30.7};

    set_value(42, "Ejemplo", 3, valores, coord);

    return 0;
}