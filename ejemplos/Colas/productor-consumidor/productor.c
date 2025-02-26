#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_BUFFER               10    /* tamaño del buffer */
#define DATOS_A_PRODUCIR         100000 /* datos a producir */

void Productor(mqd_t cola){
        int dato;
        int i;

        for(i=0;i<DATOS_A_PRODUCIR;i++){
                /* producir dato */
                dato = i;
                if (mq_send(cola, (const char *)&dato, sizeof(int), 0)== -1){
                        perror("mq_send");
                        mq_close(cola);
                        exit(1);
                }
        } /* end for */
        return;
} /* end productor */


int main(void){
        mqd_t almacen;     /* cola de mensajes donde dejar los             
		       datos producidos y extraer los datos a consumir */
        struct mq_attr attr;

        attr.mq_maxmsg = MAX_BUFFER;
        attr.mq_msgsize = sizeof (int);

        almacen = mq_open("/ALMACEN", O_CREAT|O_WRONLY, 0700, &attr);
        if (almacen == -1){
                perror ("mq_open");
                exit(-1);
        }

        Productor(almacen);
        mq_close(almacen);
        return(0);
}


