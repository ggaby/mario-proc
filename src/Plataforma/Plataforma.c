/*
 * Plataforma.c
 *
 *  Created on: 11/05/2013
 *      Author: reyiyo
 */

#include <stdio.h>
#include <stdlib.h>
#include "../common/sockets.h"

#define PORT 5000

int main(int argc, char* argv[]) {
	t_socket_server* server = sockets_createServer(NULL, PORT);
	t_socket_client* client;
	t_socket_buffer* buffer;

	sockets_listen(server);
	printf("Escuchando conexiones entrantes.\n");

	client = sockets_accept(server);
	buffer = sockets_recv(client);
	printf("Mensaje recibido: %s\n", buffer->data);
	printf("Tamanio del buffer %d bytes!\n", buffer->size);
	fflush(stdout);
	sockets_bufferDestroy(buffer);
	sockets_destroyClient(client);
	sockets_destroyServer(server);
	printf("Server cerrado correctamente.\n");

	return EXIT_SUCCESS;
}
