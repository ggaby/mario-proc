/*
 * Personaje.h
 *
 *  Created on: 10/06/2013
 *      Author: reyiyo
 */

#ifndef PERSONAJE_H_
#define PERSONAJE_H_

#include "../common/common_structs.h"
#include "../common/mensaje.h"
#include "../common/sockets.h"
#include <commons/collections/dictionary.h>
#include <commons/config.h>
#include <commons/log.h>

typedef struct {
	char* nombre;
	char simbolo;
	char** plan_de_niveles;
	t_dictionary* objetivos;
	int vidas;
	t_connection_info* orquestador;
	int puerto;
	t_log* logger;
} t_personaje;

typedef struct {
	t_connection_info* data;
	t_connection_info* planificador;
} personaje_t_nivel;

t_personaje* personaje_create(char* config_path);
void personaje_destroy(t_personaje* self);
personaje_t_nivel* personaje_nivel_create(t_connection_info* nivel, t_connection_info* planificador);
void personaje_nivel_destroy(personaje_t_nivel* self);
t_dictionary* _personaje_load_objetivos(t_config* config,
		char** plan_de_niveles);
t_connection_info* personaje_get_info_nivel(t_socket_client* orquestador);
t_socket_client* personaje_conectar_a_orquestador();

#endif /* PERSONAJE_H_ */
