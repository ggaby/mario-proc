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

t_orquestador* orquestador_create(int puerto, t_plataforma* plataforma);
void orquestador_destroy(t_orquestador* self);
void* orquestador(void* plat);
bool procesar_handshake_nivel(t_orquestador* self,
		t_socket_client* socket_nivel, t_plataforma* plataforma);
void mostrar_mensaje(t_mensaje* mensaje, t_socket_client* client);
void process_request(t_mensaje* request, t_socket_client* client,
		t_plataforma* plataforma);
void orquestador_get_info_nivel(t_mensaje* request, t_socket_client* client,
		t_plataforma* plataforma);
void orquestador_send_error_message(char* error_description,
		t_socket_client* client);
void verificar_nivel_desconectado(t_plataforma* plataforma,
		t_socket_client* client);
void orquestador_handler_deadlock(char* ids_personajes_en_deadlock,
		t_plataforma* plataforma, t_socket_client* socket_nivel);

#endif /* ORQUESTADOR_H_ */
