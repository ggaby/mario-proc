#ifndef NIVEL_H_
#define NIVEL_H_

#include <stdbool.h>
#include <pthread.h>
#include "../common/sockets.h"
#include "../common/mensaje.h"
#include "../common/posicion.h"
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include "Mapa.h"
#include "../common/Recurso.h"

#define NIVEL_CONFIG_COLUMNAS "cantidadColumnasMapa"
#define NIVEL_CONFIG_FILAS "cantidadFilasMapa"

typedef struct {
	char* nombre;
	t_list* recursos;
	t_list* personajes;
	t_connection_info* orquestador_info;
	int tiempoChequeoDeadlock;
	bool recovery;
	pthread_mutex_t logger_mutex;
	t_log* logger;
	int puerto;
	t_socket_client* socket_orquestador;
	t_list* servers;
	t_list* clients;
	t_mapa* mapa;
} nivel_t_nivel;

typedef struct {
	t_socket_client* socket;
	char id;
	t_posicion* posicion;
	t_list* recursos_asignados;
	char* simbolo_recurso_esperado;
} nivel_t_personaje;

nivel_t_nivel* nivel_create(char* config_path);
void nivel_destroy(nivel_t_nivel* self);
bool nivel_process_request(nivel_t_nivel* self, t_mensaje* request,
		t_socket_client* client);
bool nivel_conectar_a_orquestador(nivel_t_nivel* self);
void nivel_get_nombre(nivel_t_nivel* self, t_socket_client* client);
void nivel_get_posicion_recurso(nivel_t_nivel* self, char* id_recurso,
		t_socket_client* client);
void verificar_personaje_desconectado(nivel_t_nivel* self,
		t_socket_client* client, bool fin_de_nivel);
void nivel_loguear(void log_fn(t_log*, const char*, ...), nivel_t_nivel* self,
		const char* message, ...);
void nivel_create_verificador_deadlock(nivel_t_nivel* self);
void nivel_asignar_recurso(nivel_t_nivel* self, t_posicion* posicion,
		t_socket_client* client);
void asignar_recurso_a_personaje(nivel_t_nivel* self,
		nivel_t_personaje* personaje, t_recurso* recurso);
void nivel_liberar_recursos(nivel_t_nivel* self, t_list* recursos);
void nivel_destroy_personaje(nivel_t_personaje* personaje);
void avisar_al_orquestador_que_se_liberaron_recursos(nivel_t_nivel* self,
		t_list* recursos);
void nivel_asigar_recursos_liberados(nivel_t_nivel* self,
		char* recursos_asignados_str, t_socket_client* client);

#endif /* NIVEL_H_ */
