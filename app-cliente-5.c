#include <stdio.h>
#include "claves.h"

int main (int argc, char **argv)
{
int key = 234;
int err = delete_key(key);
if (err == -1) {
printf("Error al insertar la tupla\n");
 }

return 0;
}