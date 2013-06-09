/*
 * Plataforma.c
 *
 *  Created on: 11/05/2013
 *      Author: reyiyo
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "Plataforma.h"
#include <commons/string.h>

#define PORT 5000

int main(int argc, char* argv[]) {
	t_socket_server* server = sockets_createServer(NULL, PORT);

	if (!sockets_listen(server)) {
		printf("No se puede escuchar\n");
		sockets_destroyServer(server);
		return EXIT_FAILURE;
	}

	printf("Escuchando conexiones entrantes.\n");

	t_list *servers = list_create();
	list_add(servers, server);

	t_socket_client* acceptClosure(t_socket_server* server) {
		t_socket_client* client = sockets_accept(server);
		t_socket_buffer* buffer = sockets_recv(client);
		if (buffer == NULL ) {
			sockets_destroyClient(client);
			printf("Error al recibir datos en el accept\n");
			return NULL ;
		}

		t_mensaje* mensaje = mensaje_deserializer(buffer, 0);
		sockets_bufferDestroy(buffer);
		if (!handshake(client, mensaje)) {
			mensaje_destroy(mensaje);
			sockets_destroyClient(client);
			return NULL ;
		}
		mensaje_destroy(mensaje);
		return client;
	}

	int recvClosure(t_socket_client* client) {
		t_socket_buffer* buffer = sockets_recv(client);

		if (buffer == NULL ) {
			return false;
		}

		t_mensaje* mensaje = mensaje_deserializer(buffer, 0);
		//HACER ALGO
		mostrar_mensaje(mensaje, client);

		mensaje_destroy(mensaje);
		sockets_bufferDestroy(buffer);
		return true;
	}

	t_list* clients = list_create();

	while (true) {
		printf("Entro al select\n");
		sockets_select(servers, clients, 0, &acceptClosure, &recvClosure);
	}

	list_destroy_and_destroy_elements(servers, (void*) sockets_destroyServer);
	list_destroy_and_destroy_elements(clients, (void*) sockets_destroyClient);
	printf("Server cerrado correctamente.\n");

	return EXIT_SUCCESS;
}

int handshake(t_socket_client* client, t_mensaje *rq) {
	if (rq->type != M_HANDSHAKE) {
		printf("Handshake inválido!\n");
		//log_warning(log, "FSLISTENER", "Handshake invalido");
		return false;
	}
	if (!string_equals_ignore_case((char*) rq->payload, "Aquí un personaje")) {
		printf("Handshake inválido!\n");
		//log_warning(log, "FSLISTENER", "Handshake invalido");
		return false;
	}
	printf("Hanshake válido!\n");
	t_mensaje* mensaje = mensaje_create(M_HANDSHAKE);
	mensaje_setdata(mensaje, strdup("OK"), strlen("OK") + 1);
	mensaje_send(mensaje, client);
	mensaje_destroy(mensaje);
	return true;
}

void mostrar_mensaje(t_mensaje* mensaje, t_socket_client* client) {
	printf("Mensaje recibido del socket: %d", client->socket->desc);
	printf("TYPE: %d\n", mensaje->type);
	printf("LENGHT: %d\n", mensaje->length);
	printf("PAYLOAD: %s\n", (char*) mensaje->payload);
}
