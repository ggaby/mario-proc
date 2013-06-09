/*
 * Plataforma.h
 *
 *  Created on: 02/06/2013
 *      Author: test
 */

#ifndef PLATAFORMA_H_
#define PLATAFORMA_H_

#include "../common/sockets.h"
#include "../common/mensaje.h"
#include <commons/collections/list.h>

int handshake(t_socket_client* client, t_mensaje *rq);
void mostrar_mensaje(t_mensaje* mensaje, t_socket_client* client);

#endif /* PLATAFORMA_H_ */
