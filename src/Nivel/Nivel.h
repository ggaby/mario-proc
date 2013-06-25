#ifndef NIVEL_H_
#define NIVEL_H_

#include <stdbool.h>
#include "../common/sockets.h"
#include "../common/mensaje.h"
#include <commons/config.h>
#include <commons/collections/list.h>

typedef struct {
	char* nombre;
	//por el momento se va a usar una lista de recurso, pero se puede usar un diccionario de Recursos
	//char** caja_nivel;
	//t_dictionary* contenido_cajas;
	
	t_list* recursos;
	t_connection_info* orquestador; 
	int tiempoChequeoDeadlock;
	bool recovery;
}t_nivel;

typedef struct {
	char* nombre;
	char* simbolo;
	int instancia;
	int posX;
	int posY;
} t_recurso;


t_nivel* nivel_create(char* config_path);
void nivel_destroy(t_nivel* self);
int handshake(t_socket_client* client, t_mensaje *rq);

#endif /* NIVEL_H_ */
