#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_BUFFER               10    /* tamaño del buffer */
#define DATOS_A_PRODUCIR         100000 /* datos a producir */

void Consumidor(mqd_t cola){
        int dato;
        int i;

        for(i=0;i<DATOS_A_PRODUCIR;i++){
                /* recibir dato */
                dato = i;
                if (mq_receive(cola, (char *)&dato, sizeof(int),  0)== -1){
                        perror("mq_receive");
                        mq_close(cola);
                        exit(1);
                }
                /* Consumir el dato */
                printf("El dato consumido es: %d\n", dato);
        } /* end for */
        return;
} /* end consumidor */


int main(void){
        mqd_t almacen;       /* cola de mensajes donde dejar los    datos 
			producidos y extraer los datos a consumir */
        almacen = mq_open("/ALMACEN", O_RDONLY);
        if (almacen == -1){
                perror ("mq_open");
                exit(-1);
        }
        Consumidor(almacen );
        mq_close(almacen);

        return(0);
}

