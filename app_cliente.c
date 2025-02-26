#include <stdio.h>
#include "claves.h"
int main (int argc, char **argv)
{
int key = 5;
char *v1 = "ejemplo de valor 1";
double v2[] = {2.3, 0.5, 23.45};
struct Coord v3;
v3.x = 10;
v3.y = 5;
int err = set_value(key, v1, 3, v2, v3);
if (err == -1) {
printf("Error al insertar la tupla\n");
 }
return 0;
}

/*NOTAS:
->No se puede modificar claves.h
->Cliente no puede tener ninguna referencia a comunicación(colas)
->Hay que implementar los métodos que llama el cliente en el proxy-mq.c 
->Hay que hacer claves.c, servidor-mq.c y proxy-mq.case
->Hay que general una biblioteca dinámica con proxy y libclaves: Opciones del gcc
->Hacer plan de pruebas
*/