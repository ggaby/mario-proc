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

typedef struct{
	char* nombre; //id del nivel
	t_socket_client* socket_nivel;
	t_connection_info* planificador;
	pthread_t thread_planificador;
} plataforma_t_nivel;

typedef struct {
	t_log* logger;
	pthread_mutex_t logger_mutex;
	t_list* niveles;
} t_plataforma;

t_plataforma* plataforma_create();
void plataforma_destroy(t_plataforma* self);
int plataforma_create_nivel(t_plataforma* self, char* nombre_nivel, t_socket_client* socket_nivel, char* planificador_connection_info);

#endif /* PLATAFORMA_H_ */
