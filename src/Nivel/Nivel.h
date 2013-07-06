#ifndef NIVEL_H_
#define NIVEL_H_

#include <stdbool.h>
#include "../common/sockets.h"
#include "../common/mensaje.h"
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/log.h>

typedef struct {
	char* nombre;
	//por el momento se va a usar una lista de recurso, pero se puede usar un diccionario de Recursos
	//char** caja_nivel;
	//t_dictionary* contenido_cajas;
	
	t_list* recursos;
	t_connection_info* orquestador; 
	int tiempoChequeoDeadlock;
	bool recovery;
	t_log* logger;
	int puerto; //TODO tengo entendido que el puerto es un unsigned short int (ver de cambiar)
} nivel_t_nivel;

typedef struct {
	char* nombre;
	char* simbolo;
	int instancia;
	int posX;
	int posY;
} t_recurso;


nivel_t_nivel* nivel_create(char* config_path);
void nivel_destroy(nivel_t_nivel* self);
t_socket_client* nivel_conectar_a_orquestador(nivel_t_nivel* self);
void nivel_get_nombre(nivel_t_nivel* self, t_socket_client* client);

#endif /* NIVEL_H_ */
