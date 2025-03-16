#include <stdio.h>
#include "claves.h"

int main (int argc, char **argv)
{
int key = 44;
char *v1 = "klk";
double v2[] = {1.3, 0.5, 23.45};
struct Coord v3;
v3.x = 10;
v3.y = 5;
int err = modify_value(key, v1, 3, v2, v3);

if (err == -1) {
printf("Error al insertar la tupla\n");
 }

return 0;
}