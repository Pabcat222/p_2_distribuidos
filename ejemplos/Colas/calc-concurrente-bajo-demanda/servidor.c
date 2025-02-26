#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "mensaje.h"


/* mutex y variables condicionales para proteger la copia del mensaje*/
pthread_mutex_t mutex_mensaje;
int mensaje_no_copiado = true;
pthread_cond_t cond_mensaje;
mqd_t  q_servidor;

void tratar_mensaje(void  *mess){
	struct peticion mensaje;	/* mensaje local */
	mqd_t q_cliente;		/* cola del cliente */
	int resultado;		/* resultado de la operación */

	/* el thread copia el mensaje a un mensaje local */
	pthread_mutex_lock(&mutex_mensaje);

	mensaje = (*(struct peticion *) mess);

	/* ya se puede despertar al servidor*/
	mensaje_no_copiado = false;

	pthread_cond_signal(&cond_mensaje);

	pthread_mutex_unlock(&mutex_mensaje);

	/* ejecutar la petición del cliente y preparar respuesta */
	if (mensaje.op ==0)
		resultado = mensaje.a + mensaje.b;
	else
		resultado = mensaje.a - mensaje.b;
	/* Se devuelve el resultado al cliente */
	/* Para ello se envía el resultado a su cola */
	q_cliente = mq_open(mensaje.q_name, O_WRONLY);
	if (q_cliente == -1){
		perror("No se puede abrir la cola del cliente");
		mq_close(q_servidor);
		mq_unlink("/SERVIDOR_SUMA2");
	}
	else {
		if (mq_send(q_cliente, (const char *) &resultado, sizeof(int), 0) <0) {
			perror("mq_send");
			mq_close(q_servidor);
			mq_unlink("/SERVIDOR_SUMA2");
			mq_close(q_cliente);
		}
	}
	pthread_exit(0);
}


int main(void) {
        struct peticion mess;     
        struct mq_attr attr;
	pthread_attr_t t_attr;		// atributos de los threads 
   	pthread_t thid;

        attr.mq_maxmsg = 10;                
	attr.mq_msgsize = sizeof(struct peticion);

        q_servidor = mq_open("/SERVIDOR_SUMA2", O_CREAT|O_RDONLY, 0700, &attr);
	if (q_servidor == -1) {
		perror("mq_open");
		return -1;
	}

	pthread_mutex_init(&mutex_mensaje, NULL);
	pthread_cond_init(&cond_mensaje, NULL);
	pthread_attr_init(&t_attr);

	// atributos de los threads, threads independientes
	pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);

        while(1) {
                if (mq_receive(q_servidor, (char *) &mess, sizeof(mess), 0) < 0 ){
			perror("mq_recev");
			return -1;
		}

		if (pthread_create(&thid, &t_attr, (void *)tratar_mensaje, (void *)&mess)== 0) {
			// se espera a que el thread copie el mensaje 
			pthread_mutex_lock(&mutex_mensaje);
			while (mensaje_no_copiado)
				pthread_cond_wait(&cond_mensaje, &mutex_mensaje);
			mensaje_no_copiado = true;
			pthread_mutex_unlock(&mutex_mensaje);
	 	}   
        }
	return 0;
}
