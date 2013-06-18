/*
 * Personaje.h
 *
 *  Created on: 10/06/2013
 *      Author: reyiyo
 */

#ifndef PERSONAJE_H_
#define PERSONAJE_H_

#include "../common/common_structs.h"
#include <commons/collections/dictionary.h>
#include <commons/config.h>

typedef struct {
	char* nombre;
	char simbolo;
	char** plan_de_niveles;
	t_dictionary* objetivos;
	int vidas;
	t_connection_info* orquestador;
} t_personaje;

typedef struct {
	t_connection_info* data;
	t_connection_info* planificador;
} t_nivel;

t_personaje* personaje_create(char* config_path);
void personaje_destroy(t_personaje* self);
t_nivel* nivel_create(t_connection_info* nivel, t_connection_info* planificador);
void nivel_destroy(t_nivel* self);
t_dictionary* _personaje_load_objetivos(t_config* config,
		char** plan_de_niveles);

#endif /* PERSONAJE_H_ */