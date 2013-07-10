/*
 * Orquestador.h
 *
 *  Created on: 20/06/2013
 *      Author: reyiyo
 */

#ifndef ORQUESTADOR_H_
#define ORQUESTADOR_H_

#include "../Plataforma.h"
#include "../../common/sockets.h"
#include "../../common/mensaje.h"
#include <commons/collections/list.h>

typedef struct {
	t_plataforma* plataforma;
	plataforma_t_nivel* nivel;
} thread_planificador_args;

void* orquestador(void* plat);
void responder_handshake(t_socket_client* client);
bool procesar_handshake_nivel(t_socket_client* socket_nivel);
void mostrar_mensaje(t_mensaje* mensaje, t_socket_client* client);
void process_request(t_mensaje* request, t_socket_client* client, t_plataforma* plataforma);
void orquestador_get_info_nivel(t_mensaje* request, t_socket_client* client, t_plataforma* plataforma);
void orquestador_send_error_message(char* error_description, t_socket_client* client);

#endif /* ORQUESTADOR_H_ */
