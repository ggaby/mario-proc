/*
 * Plataforma.h
 *
 *  Created on: 22/06/2013
 *      Author: test
 */

#ifndef PLATAFORMA_H_
#define PLATAFORMA_H_

#include <pthread.h>
#include <commons/log.h>
#include "../common/sockets.h"
#include "../common/mensaje.h"
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>

typedef struct {
	int puerto;
	int planificadores_count;
	t_list* clients;
	t_list* servers;
} t_orquestador;

typedef struct {
	char simbolo;
	t_socket_client* socket;
	char* tiempo_llegada;
} planificador_t_personaje;

typedef struct {
	t_queue* personajes_ready;
	//El sueño del pibe: un diccionario de colas ;)
	t_dictionary* personajes_blocked;
	planificador_t_personaje* personaje_ejecutando;
	int quantum_total;
	int quantum_restante;
	unsigned int tiempo_sleep;
	t_list* servers;
	t_list* clients;
	t_connection_info* connection_info;
	char* log_name;
	//Otro alto hack: Lo meto en un socket para meterlo al select ;)
	t_socket_client* inotify_fd_wrapper;
} t_planificador;

typedef struct {
	char* nombre; //id del nivel
	t_connection_info* connection_info;
	t_socket_client* socket_nivel;
	pthread_t thread_planificador;
	t_planificador* planificador;
} plataforma_t_nivel;

typedef struct {
	t_list* niveles;
	t_orquestador* orquestador;
	t_log* logger;
	pthread_mutex_t logger_mutex;
	char* ip;
	char* config_path;
} t_plataforma;

typedef struct {
	t_plataforma* plataforma;
	t_connection_info* planificador_connection_info;
	char* nombre_nivel;
} thread_planificador_args;

t_plataforma* plataforma_create(char* config_path);
void plataforma_destroy(t_plataforma* self);
int plataforma_create_nivel(t_plataforma* self, char* nombre_nivel,
		t_socket_client* socket_nivel, char* planificador_connection_info);
void plataforma_nivel_destroy(plataforma_t_nivel* nivel);
plataforma_t_nivel* plataforma_get_nivel(t_plataforma* self, char* nombre_nivel);

#endif /* PLATAFORMA_H_ */
