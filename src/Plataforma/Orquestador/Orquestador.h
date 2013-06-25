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

void* orquestador(void* plat);
int handshake(t_socket_client* client, t_mensaje *rq);
void mostrar_mensaje(t_mensaje* mensaje, t_socket_client* client);
void process_request(t_mensaje* request, t_socket_client* client);
void orquestador_get_info_nivel(t_mensaje* request, t_socket_client* client);
void orquestador_send_error_message(char* error_description, t_socket_client* client);

#endif /* ORQUESTADOR_H_ */
