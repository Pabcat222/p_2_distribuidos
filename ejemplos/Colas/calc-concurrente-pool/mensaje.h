#define MAXSIZE	256
#define SUMA	0
#define RESTAR 1
// . . . 

struct peticion  {
	int op;			/* operación, 0 (+) 1 (-) */
	int a; 	   		/* operando 1 */
	int b;	   		/* operando 2 */
	char q_name[MAXSIZE]; 	/* nombre de la cola cliente donde debe enviar la respuesta 				 			  el servidor */
};
