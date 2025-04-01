/********************
 * proxy-sock-portable.c
 *
 * - Envía los campos “uno a uno” en big-endian (enteros)
 *   y convierte doubles a 8 bytes big-endian.
 * - Lee IP y puerto del entorno: IP_TUPLAS, PORT_TUPLAS.
 * - Llama internamente a set_value, destroy, etc. que usan
 *   send_message() para mandar la petición.
 ********************/

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include "claves.h"
 
 // Estructura de referencia
 #define MAX_BUF 256
 typedef struct {
     int operation;
     int key;
     char value1[MAX_BUF];
     int N_value2;
     double V_value2[32];
     struct Coord value3;
 } Obj;
 
 //------------------------------------------
 // Funciones de ayuda
 //------------------------------------------
 static int sendAll(int sockfd, const void *buf, size_t n)
 {
     const char *p = buf;
     size_t total = 0;
     while (total < n) {
         ssize_t s = send(sockfd, p + total, n - total, 0);
         if (s <= 0) return -1;
         total += s;
     }
     return 0;
 }
 static int recvAll(int sockfd, void *buf, size_t n)
 {
     char *p = buf;
     size_t total = 0;
     while (total < n) {
         ssize_t r = recv(sockfd, p + total, n - total, 0);
         if (r <= 0) return -1;
         total += r;
     }
     return 0;
 }
 
 static void double_to_be64(double d, unsigned char out[8])
 {
     union { double d; uint64_t u; } conv;
     conv.d = d;
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
     union { double d; uint64_t u; } conv;
     conv.u = ((uint64_t)in[0]<<56)
            | ((uint64_t)in[1]<<48)
            | ((uint64_t)in[2]<<40)
            | ((uint64_t)in[3]<<32)
            | ((uint64_t)in[4]<<24)
            | ((uint64_t)in[5]<<16)
            | ((uint64_t)in[6]<<8)
            | ((uint64_t)in[7]<<0);
     return conv.d;
 }
 
 //------------------------------------------
 // Enviar los campos del Obj uno a uno
 //------------------------------------------
 static int send_obj_fields(int sockfd, const Obj *obj)
 {
     // operation
     int32_t net_op = htonl(obj->operation);
     if (sendAll(sockfd, &net_op, 4) < 0) return -1;
 
     // key
     int32_t net_key = htonl(obj->key);
     if (sendAll(sockfd, &net_key, 4) < 0) return -1;
 
     // value1 => [int32 length + data]
     int len_v1 = (int)strlen(obj->value1);
     int32_t net_len_v1 = htonl(len_v1);
     if (sendAll(sockfd, &net_len_v1, 4) < 0) return -1;
     if (sendAll(sockfd, obj->value1, len_v1) < 0) return -1;
 
     // N_value2
     int32_t net_n = htonl(obj->N_value2);
     if (sendAll(sockfd, &net_n, 4) < 0) return -1;
 
     // V_value2 => doubles en big-endian
     for (int i=0; i<obj->N_value2; i++){
         unsigned char be[8];
         double_to_be64(obj->V_value2[i], be);
         if (sendAll(sockfd, be, 8) < 0) return -1;
     }
 
     // coord.x, coord.y
     int32_t net_x = htonl(obj->value3.x);
     int32_t net_y = htonl(obj->value3.y);
     if (sendAll(sockfd, &net_x, 4) < 0) return -1;
     if (sendAll(sockfd, &net_y, 4) < 0) return -1;
 
     return 0;
 }
 
 //------------------------------------------
 // Recibir los campos
 //------------------------------------------
 static int recv_obj_fields(int sockfd, Obj *obj)
 {
     memset(obj, 0, sizeof(*obj));
 
     // operation
     int32_t net_op;
     if (recvAll(sockfd, &net_op, 4) < 0) return -1;
     obj->operation = ntohl(net_op);
 
     // key
     int32_t net_key;
     if (recvAll(sockfd, &net_key, 4) < 0) return -1;
     obj->key = ntohl(net_key);
 
     // value1 => [length + data]
     int32_t net_len_v1;
     if (recvAll(sockfd, &net_len_v1, 4) < 0) return -1;
     int len_v1 = ntohl(net_len_v1);
     if (len_v1 < 0 || len_v1 >= MAX_BUF) return -1;
     if (recvAll(sockfd, obj->value1, len_v1) < 0) return -1;
     obj->value1[len_v1] = '\0';
 
     // N_value2
     int32_t net_n;
     if (recvAll(sockfd, &net_n, 4) < 0) return -1;
     obj->N_value2 = ntohl(net_n);
     if (obj->N_value2 < 0 || obj->N_value2 > 32) return -1;
 
     // array de doubles
     for (int i=0; i<obj->N_value2; i++){
         unsigned char be[8];
         if (recvAll(sockfd, be, 8) < 0) return -1;
         obj->V_value2[i] = be64_to_double(be);
     }
 
     // coord.x, coord.y
     int32_t net_x, net_y;
     if (recvAll(sockfd, &net_x, 4) < 0) return -1;
     if (recvAll(sockfd, &net_y, 4) < 0) return -1;
     obj->value3.x = ntohl(net_x);
     obj->value3.y = ntohl(net_y);
 
     return 0;
 }
 
 //------------------------------------------
 // Conexión al servidor (IP y puerto del entorno)
 //------------------------------------------
 static int connect_to_server(void)
 {
     char *ip_str = getenv("IP_TUPLAS");
     char *port_str = getenv("PORT_TUPLAS");
     if (!ip_str || !port_str) {
         fprintf(stderr, "[CLIENTE] Falta IP_TUPLAS o PORT_TUPLAS\n");
         return -1;
     }
     int port = atoi(port_str);
     if (port<=0) {
         fprintf(stderr, "[CLIENTE] Puerto inválido\n");
         return -1;
     }
     int sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd<0) {
         perror("[CLIENTE] socket");
         return -1;
     }
     struct sockaddr_in serv_addr;
     memset(&serv_addr,0,sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_port   = htons(port);
     if (inet_pton(AF_INET, ip_str, &serv_addr.sin_addr)<=0) {
         fprintf(stderr, "[CLIENTE] IP inválida: %s\n", ip_str);
         close(sockfd);
         return -1;
     }
     if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0) {
         perror("[CLIENTE] connect");
         close(sockfd);
         return -1;
     }
     printf("[CLIENTE] Conectado a %s:%d\n", ip_str, port);
     return sockfd;
 }
 
 //------------------------------------------
 // Función genérica: envía peticion, recibe respuesta
 //------------------------------------------
 static int send_message(const Obj *req, Obj *resp)
 {
     int sockfd = connect_to_server();
     if (sockfd<0) return -1;
 
     // Enviar
     if (send_obj_fields(sockfd, req) < 0) {
         close(sockfd);
         return -1;
     }
 
     // Recibir
     if (recv_obj_fields(sockfd, resp) < 0) {
         close(sockfd);
         return -1;
     }
 
     close(sockfd);
     printf("[CLIENTE] Respuesta del servidor: %d\n", resp->operation);

     return 0;
 }
 
 //------------------------------------------
 // Ahora definimos set_value, destroy, etc.
 //------------------------------------------
 int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord c)
 {
     if (N_value2 < 1 || N_value2>32) return -1;
 
     Obj req, resp;
     memset(&req,0,sizeof(req));
     req.operation = 1;
     req.key       = key;
     strncpy(req.value1, value1, sizeof(req.value1)-1);
     req.N_value2 = N_value2;
     memcpy(req.V_value2, V_value2, N_value2*sizeof(double));
     req.value3   = c;
 
     if (send_message(&req, &resp)<0) return -1;
     return (resp.operation==0) ? 0 : -1;
 }
 
 int destroy(void)
 {
     Obj req, resp;
     memset(&req,0,sizeof(req));
     req.operation=2;
     if (send_message(&req, &resp)<0) return -1;
     return (resp.operation==0)?0:-1;
 }
 
 int delete_key(int key)
 {
     Obj req,resp; memset(&req,0,sizeof(req));
     req.operation=3;
     req.key=key;
     if (send_message(&req,&resp)<0) return -1;
     return (resp.operation==0)?0:-1;
 }
 
 int exist(int key)
 {
     Obj req,resp; memset(&req,0,sizeof(req));
     req.operation=4;
     req.key=key;
     if (send_message(&req,&resp)<0) return -1; // error en comunicación
     return resp.operation; // -1=error, 0=no existe, 1=existe
 }
 
 int modify_value(int key, char *value1, int N_value2, double *V_value2, struct Coord c)
 {
     if(N_value2<1||N_value2>32) return -1;
     Obj req,resp; memset(&req,0,sizeof(req));
     req.operation=5;
     req.key=key;
     strncpy(req.value1,value1,sizeof(req.value1)-1);
     req.N_value2=N_value2;
     memcpy(req.V_value2,V_value2,N_value2*sizeof(double));
     req.value3=c;
     if (send_message(&req,&resp)<0) return -1;
     return (resp.operation==0)?0:-1;
 }
 
 int get_value(int key, char *value1, int *N_value2, double *V_value2, struct Coord *c)
 {
     Obj req,resp; memset(&req,0,sizeof(req));
     req.operation=6;
     req.key=key;
 
     if (send_message(&req,&resp)<0) return -1;
     if (resp.operation==0){
         strcpy(value1, resp.value1);
         *N_value2= resp.N_value2;
         memcpy(V_value2, resp.V_value2, resp.N_value2*sizeof(double));
         *c= resp.value3;
         return 0;
     }
     return -1;
 }