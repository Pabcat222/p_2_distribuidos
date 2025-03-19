#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdbool.h>
#include "claves.h"
#include <signal.h>
#include <unistd.h>
//
// Declaración de instancias relacionadas con mutex y threads
mqd_t q_servidor;
pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_mensaje;
pthread_cond_t cond_mensaje;
int mensaje_no_copiado = true;

// Estructura objeto, lo que se envía y recibe por las colas
typedef struct {
    int key;
    char value1[256];
    int N_value2;
    double V_value2[32];
    struct Coord value3;
    int operation;
    char q_name[256];
} Obj;   

// Función para tratar y enviar mensajes a los clientes, recibe como argumento el mensaje que le pasa el cliente por la q_servidor
int tratarMensaje(void *mess){
    Obj obj;
    Obj msg;
    int res; // Respuesta
    mqd_t q_cliente;

    // Thread copia mensaje a mensaje local
    pthread_mutex_lock(&mutex_mensaje);
    obj = (*(Obj *) mess);

    // Ya se puede despertar al servidor
    mensaje_no_copiado = false;
    pthread_cond_signal(&cond_mensaje);
    pthread_mutex_unlock(&mutex_mensaje);

    // Ejecución de las distintas funciones del servidor dependiendo de la operación que recibe del cliente
    pthread_mutex_lock(&db_mutex);
    switch (obj.operation)
    {
    case 1: // set_value
        // Imprimir los valores recibidos
        printf("[SERVIDOR] Mensaje recibido:\n");
        printf("  key: %d\n", obj.key);
        printf("  value1: %s\n", obj.value1);
        printf("  N_value2: %d\n", obj.N_value2);
        printf("  V_value2: ");
        for (int i = 0; i < obj.N_value2; i++) {
            printf("%.2f ", obj.V_value2[i]);
        }
        printf("\n  value3: (%d, %d)\n", obj.value3.x, obj.value3.y);
        printf("  operacion: %d\n", obj.operation);

        // Insertar en la base de datos y prepara respuesta para cliente
        
        if (set_value(obj.key, obj.value1, obj.N_value2, obj.V_value2, obj.value3) == -1) {
            printf("[SERVIDOR] Error: No se pudo guardar el mensaje en la base de datos.\n");
            res = -1;
        } else {
            printf("[SERVIDOR] Mensaje guardado correctamente en la base de datos.\n");
            res = 0;
            
        }
        
        break;

    case 2: // destroy
        if (destroy()==-1){
            printf("[SERVIDOR]Fallos al eliminar la base de datos\n");
            res = -1;
        }
        else{
            res = 0;
        }
        break;

    case 3: // delete_key
        if (delete_key(obj.key) == -1){
            printf("[SERVIDOR]Fallo al eliminar los elementos de la key\n");
            
            res = -1;
        }
        else{
            
            res = 0;
        }
        break;

    case 4: // exist
        int resultado = exist(obj.key);
        if (resultado == 1){
            
            printf("[SERVIDOR]Existen datos con la key %d\n", obj.key);
            res = 1;
            }
        if (resultado == 0){
            
            printf("[SERVIDOR]No existen datos con la key %d\n", obj.key);
            res = 0;
            }
        if (resultado == -1){   
            res = -1; 
        }
        break;
            

    case 5: // modify_value
        if (exist(obj.key) == -1){
            printf("[SERVIDOR]Error en la consulta del dato\n");
            res = -1;
            }
        else{
            if (modify_value(obj.key, obj.value1, obj.N_value2, obj.V_value2, obj.value3) == -1){
                printf("[SERVIDOR]Fallo al ejecutar el modify value\n");
                res = -1;
            }
            else{
                printf("[SERVIDOR]Datos modificados correctamente\n");
                res = 0;
            }
            }
            break;

    case 6: // get_value
        char value1[256];
        int N_value2;
        double V_value2[32];
        struct Coord value3;
        if (get_value(obj.key, value1, &N_value2, V_value2, &value3) == 0) {
            printf("[SERVIDOR] Valores obtenidos:\n");
            printf("Value1: %s\n", value1);
            printf("N_value2: %d\n", N_value2);
            printf("V_value2: ");
            for (int i = 0; i < N_value2; i++) {
                printf("%.2f ", V_value2[i]);
            }
            printf("\nValue3: x=%d, y=%d\n", value3.x, value3.y);
            msg.key = obj.key;
            strcpy(msg.value1, value1);
            msg.N_value2 = N_value2;
            memcpy(msg.V_value2, V_value2, N_value2 * sizeof(double));
            msg.value3 = value3;
            strcpy(msg.q_name, obj.q_name);
            printf("[SERVIDOR] Este es el nombre de la cola del cliente: %s \n", msg.q_name);
            res = 0;
        } else {
            printf("[SERVIDOR] Error al obtener los valores para la clave %d.\n", obj.key);
        }

        break;

    }


    // Se devuelve el resultado al cliente
    q_cliente = mq_open(obj.q_name, O_WRONLY);
    
    if (q_cliente == (mqd_t)-1) {
        perror("[SERVIDOR] Error al abrir la cola de mensajes del cliente\n");
        return -2;
    }
    else{
        msg.operation = res;
        strcpy(msg.q_name, obj.q_name);
        if(mq_send(q_cliente, (char *)&msg, sizeof(Obj), 0) < 0){
            perror("[SERVIDOR] Error al enviar mensaje al cliente\n");
            mq_close(q_servidor);
            mq_unlink("/SERVIDOR-5764-5879");
            mq_close(q_cliente);

        }
    }

    pthread_mutex_unlock(&db_mutex);
    mq_close(q_cliente);
    pthread_exit(0);
}

// Función cuando se hace Ctrl + C en la terminal y se cierra el servidor
void cerrar_servidor() {
    printf("\nSaliendo del servidor...\nHasta la próxima\n");
    mq_close(q_servidor);
    mq_unlink("/SERVIDOR-5764-5879");
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
    
    // Se abre la cola servidor por la cual los clientes envían mensajes al servidor
    q_servidor = mq_open("/SERVIDOR-5764-5879", O_RDONLY | O_CREAT, 0700, &attr);
    if (q_servidor == -1) {
        perror("[SERVIDOR] Error al abrir la cola de mensajes");
        return -2;
    }

    printf("[SERVIDOR] Servidor en espera de mensajes...\n");
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