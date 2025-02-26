#include "mensaje.h"
#include <mqueue.h>
#include <string.h>
#include <stdio.h>

int main(void) {
        mqd_t q_servidor;       /* cola de mensajes del proceso servidor */
        mqd_t q_cliente;        /* cola de mensajes para el proceso cliente */

        struct peticion pet;
        int res;        struct mq_attr attr;

        attr.mq_maxmsg = 1;     attr.mq_msgsize = sizeof(int);
        q_cliente = mq_open("/CLIENTE_UNO", O_CREAT|O_RDONLY, 0700, &attr);
	if (q_cliente == -1) {
		perror("mq_open");
		return -1;
	}
        q_servidor = mq_open("/SERVIDOR_SUMA", O_WRONLY);
	if (q_servidor == -1){
		mq_close(q_cliente);
		perror("mq_open");
		return -1;
	}


        /* se rellena la petición */
        pet.op = 0; 
	pet.a = 5;  
	pet.b = 2;   
	strcpy(pet.q_name, "/CLIENTE_UNO");

        if (mq_send(q_servidor, (const char *)&pet, sizeof(pet), 0) < 0){
		perror("mq_send");
		return -1;
	}	
        if (mq_receive(q_cliente, (char *) &res, sizeof(int), 0) < 0){
		perror("mq_recv");
		return -1;
	}	

        printf("Resultado = %d\n", res);

        mq_close(q_servidor);
        mq_close(q_cliente);
        mq_unlink("/CLIENTE_UNO");
	return 0;
}

