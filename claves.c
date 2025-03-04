#include <stdio.h>
#include "claves.h"
#include <mqueue.h>
#include <string.h>
#include <sys/stat.h>


#define MAX_BUFFER   10000

mqd_t mqdes;

typedef struct Obj {
    int key;
    char *value1;
    int N_value2;
    double *V_value2;
    struct Coord value3;
}Obj;

int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3){
    mqdes = mqopen ("/Cola1", O_WRONLY|O_CREAT, 0664, NULL);
    if (mqdes == (mqd_t)-1) {
        perror("Error al abrir la cola de mensajes");
        return -1;
    }
    
    Obj mensaje;
    mensaje.key = key;

    mensaje.value1 = malloc(strlen(value1) + 1); //Reservas memoria con la misma longitud que la cadena de caracteres
    if (mensaje.value1 == NULL) {
        perror("No se pudo asignar memoria para la cadena de caracteres");
        return -1;
    }
    strcpy(mensaje.value1, value1);

    mensaje.N_value2 = N_value2;
    mensaje.V_value2 = malloc(N_value2 * sizeof(double));
    if (mensaje.V_value2 == NULL) {
        perror("Error al asignar memoria para V_value2");
        free(obj.value1); // Liberamos memoria de value1 antes de salir
        return -1;
    }
    memcpy(mensaje.V_value2, V_value2, N_value2 * sizeof(double));

    // Asignar value3
    mensaje.value3 = value3;

    // Enviar la estructura a la cola de mensajes
    if (mq_send(mqdes, (char *)&mensaje, sizeof(mensaje), 0) == -1) {
        perror("Error al enviar el mensaje");
        free(mensaje.value1);
        free(mensaje.V_value2);
        return -1;
    }

    printf("Objeto enviado correctamente a la cola de mensajes.\n");

    // Liberar memoria y cerrar la cola
    free(mensaje.value1);
    free(mensaje.V_value2);
    mq_close(mqdes);

    return 0;
}

