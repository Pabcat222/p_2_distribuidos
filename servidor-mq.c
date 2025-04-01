/************************************************************
 * servidor-sock-portable.c
 *
 * - Recibe y envía campos “uno a uno” en un orden fijo.
 * - Los enteros se convierten con htonl/ntohl (big-endian).
 * - Los doubles se transforman manualmente a 8 bytes big-endian.
 * - Usa threads para atender múltiples conexiones.
 * - Llama a funciones de "claves.h" (set_value, destroy, etc.).
 ************************************************************/

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <signal.h>
 #include <pthread.h>
 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include <sys/socket.h>
 #include "claves.h"
 
 // Estructura de referencia (NO la enviamos directamente)
 #define MAX_BUF 256
 typedef struct {
     int operation;         // 1=set_value, etc.
     int key;
     char value1[MAX_BUF];
     int N_value2;
     double V_value2[32];
     struct Coord value3;   // { int x, y; }
 } Obj;
 
 // Mutex para proteger la base de datos
 pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;
 static int stop_server = 0;
 static int server_fd = -1;
 
 //---------------------------------------------------------------------------
 // Funciones de ayuda para enviar/recibir EXACTAMENTE n bytes
 //---------------------------------------------------------------------------
 static int recvAll(int sockfd, void *buf, size_t n)
 {
     char *p = buf;
     size_t total = 0;
     while (total < n) {
         ssize_t r = recv(sockfd, p + total, n - total, 0);
         if (r <= 0) {
             return -1; // error o conexión cerrada
         }
         total += r;
     }
     return 0;
 }
 static int sendAll(int sockfd, const void *buf, size_t n)
 {
     const char *p = buf;
     size_t total = 0;
     while (total < n) {
         ssize_t s = send(sockfd, p + total, n - total, 0);
         if (s <= 0) {
             return -1;
         }
         total += s;
     }
     return 0;
 }
 
 //---------------------------------------------------------------------------
 // Conversiones de double a 8 bytes big-endian y viceversa (IEEE 754)
 //---------------------------------------------------------------------------
 static void double_to_be64(double d, unsigned char out[8])
 {
     union {
         double d;
         uint64_t u;
     } conv;
     conv.d = d;
 
     // Reordenar bytes a big-endian
     out[0] = (conv.u >> 56) & 0xFF;
     out[1] = (conv.u >> 48) & 0xFF;
     out[2] = (conv.u >> 40) & 0xFF;
     out[3] = (conv.u >> 32) & 0xFF;
     out[4] = (conv.u >> 24) & 0xFF;
     out[5] = (conv.u >> 16) & 0xFF;
     out[6] = (conv.u >>  8) & 0xFF;
     out[7] = (conv.u >>  0) & 0xFF;
 }
 
 static double be64_to_double(const unsigned char in[8])
 {
     union {
         double d;
         uint64_t u;
     } conv;
 
     conv.u = ((uint64_t)in[0] << 56)
            | ((uint64_t)in[1] << 48)
            | ((uint64_t)in[2] << 40)
            | ((uint64_t)in[3] << 32)
            | ((uint64_t)in[4] << 24)
            | ((uint64_t)in[5] << 16)
            | ((uint64_t)in[6] <<  8)
            | ((uint64_t)in[7] <<  0);
 
     return conv.d;
 }
 
 //---------------------------------------------------------------------------
 // Recibir los campos de un Obj “uno a uno”
 //---------------------------------------------------------------------------
 static int recv_obj_fields(int sockfd, Obj *obj)
 {
     memset(obj, 0, sizeof(*obj));
 
     // 1) operation (int32 big-endian)
     int32_t net_op;
     if (recvAll(sockfd, &net_op, 4) < 0) return -1;
     obj->operation = ntohl(net_op);
 
     // 2) key (int32 big-endian)
     int32_t net_key;
     if (recvAll(sockfd, &net_key, 4) < 0) return -1;
     obj->key = ntohl(net_key);
 
     // 3) value1 => primero int32 length, luego tantos bytes
     int32_t net_len;
     if (recvAll(sockfd, &net_len, 4) < 0) return -1;
     int len_v1 = ntohl(net_len);
     if (len_v1 < 0 || len_v1 >= MAX_BUF) return -1;
     if (recvAll(sockfd, obj->value1, len_v1) < 0) return -1;
     obj->value1[len_v1] = '\0';
 
     // 4) N_value2
     int32_t net_n;
     if (recvAll(sockfd, &net_n, 4) < 0) return -1;
     obj->N_value2 = ntohl(net_n);
     if (obj->N_value2 < 0 || obj->N_value2 > 32) return -1;
 
     // 5) Recibir obj->N_value2 doubles en big-endian
     for (int i=0; i<obj->N_value2; i++){
         unsigned char be[8];
         if (recvAll(sockfd, be, 8) < 0) return -1;
         obj->V_value2[i] = be64_to_double(be);
     }
 
     // 6) coord.x, coord.y
     int32_t net_x, net_y;
     if (recvAll(sockfd, &net_x, 4) < 0) return -1;
     if (recvAll(sockfd, &net_y, 4) < 0) return -1;
     obj->value3.x = ntohl(net_x);
     obj->value3.y = ntohl(net_y);
 
     return 0;
 }
 
 //---------------------------------------------------------------------------
 // Enviar los campos de un Obj “uno a uno”
 //---------------------------------------------------------------------------
 static int send_obj_fields(int sockfd, const Obj *obj)
 {
     // 1) operation
     int32_t net_op = htonl(obj->operation);
     if (sendAll(sockfd, &net_op, 4) < 0) return -1;
 
     // 2) key
     int32_t net_key = htonl(obj->key);
     if (sendAll(sockfd, &net_key, 4) < 0) return -1;
 
     // 3) value1
     int len_v1 = (int)strlen(obj->value1);
     int32_t net_len_v1 = htonl(len_v1);
     if (sendAll(sockfd, &net_len_v1, 4) < 0) return -1;
     if (sendAll(sockfd, obj->value1, len_v1) < 0) return -1;
 
     // 4) N_value2
     int32_t net_n = htonl(obj->N_value2);
     if (sendAll(sockfd, &net_n, 4) < 0) return -1;
 
     // 5) V_value2 con conversión big-endian de double
     for (int i=0; i<obj->N_value2; i++){
         unsigned char be[8];
         double_to_be64(obj->V_value2[i], be);
         if (sendAll(sockfd, be, 8) < 0) return -1;
     }
 
     // 6) coord.x, coord.y
     int32_t net_x = htonl(obj->value3.x);
     int32_t net_y = htonl(obj->value3.y);
     if (sendAll(sockfd, &net_x, 4) < 0) return -1;
     if (sendAll(sockfd, &net_y, 4) < 0) return -1;
 
     return 0;
 }
 
 //---------------------------------------------------------------------------
 // Hilo que atiende la conexión
 //---------------------------------------------------------------------------
 static void *thread_conn(void *arg)
 {
     int client_fd = *(int*)arg;
     free(arg);
 
     // Recibir la petición
     Obj peticion;
     if (recv_obj_fields(client_fd, &peticion) < 0) {
         printf("[SERVIDOR] Error recibiendo campos.\n");
         close(client_fd);
         return NULL;
     }
 
     // Procesar
     pthread_mutex_lock(&db_mutex);
 
     int res = 0;
     Obj respuesta;
     memset(&respuesta, 0, sizeof(respuesta));
 
     switch (peticion.operation) {
         case 1: // set_value
             printf("[SERVIDOR] set_value(key=%d)\n", peticion.key);
             res = set_value(peticion.key, peticion.value1,
                             peticion.N_value2, peticion.V_value2,
                             peticion.value3);
             break;
 
         case 2: // destroy
             printf("[SERVIDOR] destroy\n");
             res = destroy();
             break;
 
         case 3: // delete_key
             printf("[SERVIDOR] delete_key\n");
             res = delete_key(peticion.key);
             break;
 
         case 4: // exist
             printf("[SERVIDOR] exist(key=%d)\n", peticion.key);
             res = exist(peticion.key); 
             break;
 
         case 5: // modify_value
             printf("[SERVIDOR] modify_value\n");
             res = modify_value(peticion.key, peticion.value1,
                                peticion.N_value2, peticion.V_value2,
                                peticion.value3);
             break;
 
         case 6: // get_value
             printf("[SERVIDOR] get_value(key=%d)\n", peticion.key);
             {
                 char v1[256];
                 int n;
                 double arr[32];
                 struct Coord c;
                 int ret = get_value(peticion.key, v1, &n, arr, &c);
                 if (ret == 0) {
                     strcpy(respuesta.value1, v1);
                     respuesta.N_value2 = n;
                     memcpy(respuesta.V_value2, arr, n*sizeof(double));
                     respuesta.value3 = c;
                     res = 0;
                 } else {
                     res = -1;
                 }
             }
             break;
 
         default:
             printf("[SERVIDOR] Operación desconocida: %d\n", peticion.operation);
             res = -1;
             break;
     }
     pthread_mutex_unlock(&db_mutex);
 
     // Construir respuesta
     respuesta.operation = res;
     respuesta.key       = peticion.key;
 
     // Enviar la respuesta
     if (send_obj_fields(client_fd, &respuesta) < 0) {
         printf("[SERVIDOR] Error enviando respuesta.\n");
     }
 
     close(client_fd);
     return NULL;
 }
 
 //---------------------------------------------------------------------------
 // Manejo de Ctrl+C
 //---------------------------------------------------------------------------
 static void sig_handler(int s)
 {
     (void)s;
     printf("[SERVIDOR] Señal de cierre\n");
     stop_server = 1;
     close(server_fd);
 }
 
 //---------------------------------------------------------------------------
 // main
 //---------------------------------------------------------------------------
 int main(int argc, char *argv[])
 {
     if (argc < 2) {
         fprintf(stderr, "Uso: %s <PUERTO>\n", argv[0]);
         return 1;
     }
     int port = atoi(argv[1]);
     signal(SIGINT, sig_handler);
 
     // Crear socket
     server_fd = socket(AF_INET, SOCK_STREAM, 0);
     if (server_fd < 0) {
         perror("[SERVIDOR] socket");
         return 1;
     }
 
     // Bind
     struct sockaddr_in serv_addr;
     memset(&serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family      = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port        = htons(port);
 
     if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
         perror("[SERVIDOR] bind");
         close(server_fd);
         return 1;
     }
 
     // Listen
     if (listen(server_fd, 5) < 0) {
         perror("[SERVIDOR] listen");
         close(server_fd);
         return 1;
     }
     printf("[SERVIDOR] Esperando conexiones en el puerto %d...\n", port);
 
     // bucle accept
     while (!stop_server) {
         struct sockaddr_in client_addr;
         socklen_t addrlen = sizeof(client_addr);
         int cfd = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);
         if (cfd < 0) {
             if (stop_server) break;
             perror("[SERVIDOR] accept");
             continue;
         }
 
         // Crear hilo
         pthread_t tid;
         int *arg = malloc(sizeof(int));
         *arg = cfd;
         if (pthread_create(&tid, NULL, thread_conn, arg) != 0) {
             perror("[SERVIDOR] pthread_create");
             close(cfd);
             free(arg);
             continue;
         }
         pthread_detach(tid);
     }
 
     close(server_fd);
     printf("[SERVIDOR] Finalizado.\n");
     return 0;
 }
 