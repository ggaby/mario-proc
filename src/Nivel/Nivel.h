#ifndef NIVEL_H_
#define NIVEL_H_

#include <stdbool.h>
#include "../common/sockets.h"
#include "../common/mensaje.h"
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include "Mapa.h"

#define NIVEL_CONFIG_COLUMNAS "cantidadColumnasMapa"
#define NIVEL_CONFIG_FILAS "cantidadFilasMapa"

typedef struct {
	char* nombre;
	t_list* recursos;  //TODO
	t_list* personajes; //TODO
	t_connection_info* orquestador_info; 
	int tiempoChequeoDeadlock;
	bool recovery;
	t_log* logger;
	int puerto;
	t_socket_client* socket_orquestador;
	t_list* servers;
	t_list* clients;
	t_mapa* mapa;
} nivel_t_nivel;

typedef struct {
	char* nombre;
	char simbolo;
	int cantidad;
	int posX;
	int posY;
} t_recurso;

typedef struct {
	t_socket_client* socket;
	char id;
	int posX;
	int posY;
} nivel_t_personaje;


nivel_t_nivel* nivel_create(char* config_path);
void nivel_destroy(nivel_t_nivel* self);
bool nivel_conectar_a_orquestador(nivel_t_nivel* self);
void nivel_get_nombre(nivel_t_nivel* self, t_socket_client* client);
void verificar_personaje_desconectado(nivel_t_nivel* self,
		t_socket_client* client);

#endif /* NIVEL_H_ */
