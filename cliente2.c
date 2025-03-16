#include <stdio.h>
#include "claves.h"

int main (int argc, char **argv)
{
int key = 234;
/* char *v1 = "alohomora";
double v2[] = {2.3, 7.5, 23.45};
struct Coord v3;
v3.x = 10;
v3.y = 5;
int err = modify_value(key, v1, 3, v2, v3); */

// Para probar el get_value
char value1[256];
int N_value2;
double V_value2[32];
struct Coord value3;
int err = get_value(key, value1, &N_value2, V_value2, &value3);

if (err == -1) {
printf("Error al insertar la tupla\n");
 }

return 0;
}