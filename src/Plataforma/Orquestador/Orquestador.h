/*
 * Orquestador.h
 *
 *  Created on: 20/06/2013
 *      Author: reyiyo
 */

#ifndef ORQUESTADOR_H_
#define ORQUESTADOR_H_

#include "../../common/sockets.h"
#include "../../common/mensaje.h"
#include <commons/collections/list.h>

typedef struct{
	char* nombre; //id del nivel
	t_socket_client* nivel;
	t_connection_info* planificador;
} orquestador_t_nivel;

typedef struct{
	t_list* niveles;
} t_orquestador;

void* orquestador(void* plat);
//int handshake(t_socket_client* client, t_mensaje *rq);
void responder_handshake(t_socket_client* client);
void procesar_handshake_nivel(t_orquestador* self, t_socket_client* socket_nivel);
void mostrar_mensaje(t_mensaje* mensaje, t_socket_client* client);
void process_request(t_mensaje* request, t_socket_client* client);
t_orquestador* orquestador_create();
void orquestador_destroy(t_orquestador* self);
void orquestador_get_info_nivel(t_mensaje* request, t_socket_client* client);
void orquestador_send_error_message(char* error_description, t_socket_client* client);
orquestador_t_nivel* orquestador_create_add_nivel(t_orquestador* self, char* nombre_nivel, t_socket_client* nivel, char* planificador_connection_info);

#endif /* ORQUESTADOR_H_ */
