#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdbool.h>
#include "claves.h"
#include <signal.h>
#include <unistd.h>

mqd_t q_servidor;
pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_mensaje;
pthread_cond_t cond_mensaje;
int mensaje_no_copiado = true;


typedef struct {
    int key;
    char value1[256];
    int N_value2;
    double V_value2[32];
    struct Coord value3;
    int operation;
    char q_name[256];
} Obj;   

int tratarMensaje(void *mess){
    Obj obj;
    int res; //respuesta
    mqd_t q_cliente;

    //Thread copia mensaje a mensaje local
    pthread_mutex_lock(&mutex_mensaje);
    obj = (*(Obj *) mess);

    //Ya se puede despertar al servidor
    mensaje_no_copiado = false;
    pthread_cond_signal(&cond_mensaje);
    pthread_mutex_unlock(&mutex_mensaje);

    //EJECUCIÓN DE FUNCIONES
    pthread_mutex_lock(&db_mutex);
    switch (obj.operation)
    {
    case 1:
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

        //Insertar en la base de datos y prepara respuesta para cliente
        
        if (set_value(obj.key, obj.value1, obj.N_value2, obj.V_value2, obj.value3) == -1) {
            printf("Error: No se pudo guardar el mensaje en la base de datos.\n");
            res = -1;
        } else {
            printf("Mensaje guardado correctamente en la base de datos.\n");
            res = 0;
            
        }
        
        break;
        


    case 2:
        if (destroy()==-1){
            printf("Fallos al eliminar la base de datos\n");
            res = -1;
        }
        else{
            res = 0;
        }
        break;
    }
    pthread_mutex_unlock(&db_mutex);

    //Se devuelve el resultado al cliente
    q_cliente = mq_open(obj.q_name, O_WRONLY);
    
    if (q_cliente == (mqd_t)-1) {
        perror("Error al abrir la cola de mensajes del cliente\n");
        return -2;
    }
    else{
        if(mq_send(q_cliente, (const char *) &res, sizeof(int), 0) < 0){
            perror("Error al enviar mensaje al cliente\n");
            /*mq_close(q_servidor);
            mq_unlink("/SERVIDOR-5764-5879");
            mq_close(q_cliente);*/

        }
    }
    /*mq_close(q_servidor);
    mq_unlink("/SERVIDOR-5764-5879");
    mq_close(q_cliente);
    pthread_exit(0);*/
}
void cerrar_servidor() {
    printf("\nSaliendo del servidor...\nHasta la próxima\n");
    exit(0);}

int main() {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(Obj);
    attr.mq_curmsgs = 0;
    Obj mess;
    pthread_attr_t t_attr;
    pthread_t thid;
    signal(SIGINT, cerrar_servidor);
    
    q_servidor = mq_open("/SERVIDOR-5764-5879", O_RDONLY | O_CREAT, 0700, &attr);
    if (q_servidor == -1) {
        perror("Error al abrir la cola de mensajes");
        return -2;
    }

    printf("Servidor en espera de mensajes...\n");
    //Inician los hilos
    pthread_mutex_init(&mutex_mensaje, NULL);
    pthread_mutex_init(&db_mutex, NULL);
    pthread_cond_init(&cond_mensaje, NULL);
    pthread_attr_init(&t_attr);

    //Atributos de los threads independientes
    pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);

    while (1) { 
        ssize_t bytes_read = mq_receive(q_servidor, (char *)&mess, sizeof(Obj), NULL);
        if (bytes_read == -1) {
            perror("Error al recibir mensaje");
            continue;
        }

        if(pthread_create(&thid,&t_attr, (void *)tratarMensaje, (void *)&mess)== 0){
            //se espera a que el thread se copie el mensaje
            pthread_mutex_lock(&mutex_mensaje);
            while(mensaje_no_copiado){
                pthread_cond_wait(&cond_mensaje, &mutex_mensaje);
            }
            mensaje_no_copiado = true;
            pthread_mutex_unlock(&mutex_mensaje);
            
        }

    }
    return 0;
}