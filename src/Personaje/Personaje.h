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
#include "../common/posicion.h"
#include <commons/collections/dictionary.h>
#include <commons/config.h>
#include <commons/log.h>

typedef struct {
	char* nombre;
	t_connection_info* nivel;
	t_connection_info* planificador;
	t_socket_client* socket_nivel;
	t_socket_client* socket_planificador;
} t_personaje_nivel;

typedef struct {
	char* nombre;
	char simbolo;
	char** plan_de_niveles;
	t_dictionary* objetivos;
	int vidas;
	t_connection_info* orquestador_info;
	t_socket_client* socket_orquestador;
	int puerto;
	t_log* logger;
	t_personaje_nivel* nivel_actual;
	t_posicion* posicion;
	t_posicion* posicion_objetivo;
} t_personaje;

t_personaje* personaje_create(char* config_path);
void personaje_destroy(t_personaje* self);
t_personaje_nivel* personaje_nivel_create(char* nombre_nivel);
void personaje_nivel_destroy(t_personaje_nivel* self);
t_dictionary* _personaje_load_objetivos(t_config* config,
		char** plan_de_niveles);
bool personaje_get_info_nivel(t_personaje* self);
bool personaje_conectar_a_orquestador(t_personaje* self);
bool personaje_conectar_a_nivel(t_personaje* self);
bool personaje_conectar_a_planificador(t_personaje* self);
bool personaje_jugar_nivel(t_personaje* self);
t_posicion* pedir_posicion_objetivo(t_personaje* self, char* objetivo);
bool realizar_movimiento(t_personaje* self);
bool mover_en_nivel(t_personaje* self);
bool finalizar_turno(t_personaje* self, char* objetivo);
t_mensaje* solicitar_recurso(t_personaje* self);

#endif /* PERSONAJE_H_ */
