/*
 * Orquestador.c
 *
 *  Created on: 20/06/2013
 *      Author: reyiyo
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <commons/string.h>
#include "Orquestador.h"

#define PUERTO_ORQUESTADOR 5000

void* orquestador(void* name) {
	t_socket_server* server = sockets_createServer(NULL, PUERTO_ORQUESTADOR);

	if (!sockets_listen(server)) {
		printf("Orquestador: No se puede escuchar\n");
		sockets_destroyServer(server);
	}

	printf("Orquestador: Escuchando conexiones entrantes.\n");

	t_list *servers = list_create();
	list_add(servers, server);

	t_socket_client* acceptClosure(t_socket_server* server) {
		t_socket_client* client = sockets_accept(server);
		t_socket_buffer* buffer = sockets_recv(client);
		if (buffer == NULL ) {
			sockets_destroyClient(client);
			printf("Orquestador: Error al recibir datos en el accept\n");
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
		mostrar_mensaje(mensaje, client);
		process_request(mensaje, client);

		mensaje_destroy(mensaje);
		sockets_bufferDestroy(buffer);
		return true;
	}

	t_list* clients = list_create();

	while (true) {
		printf("Orquestador: Entro al select\n");
		sockets_select(servers, clients, 0, &acceptClosure, &recvClosure);
	}

	list_destroy_and_destroy_elements(servers, (void*) sockets_destroyServer);
	list_destroy_and_destroy_elements(clients, (void*) sockets_destroyClient);
	printf("Orquestador: Server cerrado correctamente.\n");
	return (void*) EXIT_SUCCESS;

}

void process_request(t_mensaje* request, t_socket_client* client) {
	if (request->type == M_GET_INFO_NIVEL_REQUEST) {
		return orquestador_get_info_nivel(request, client);
	} else {
		printf("Orquestador: Request desconocido: %d", request->type);
		return orquestador_send_error_message("Request desconocido", client);
	}
}

void orquestador_get_info_nivel(t_mensaje* request, t_socket_client* client) {
	char* nivel = (char*) request->payload;
	t_mensaje* response = mensaje_create(M_GET_INFO_NIVEL_RESPONSE);
	mensaje_setdata(response, string_duplicate("213.456.789.123:8080"), strlen("213.456.789.123:8080") +1);
	mensaje_send(response, client);
	mensaje_destroy(response);
}

void orquestador_send_error_message(char* error_description, t_socket_client* client) {
	t_mensaje* response = mensaje_create(M_ERROR);
	mensaje_setdata(response, strdup(error_description), strlen(error_description) + 1);
	mensaje_send(response, client);
	mensaje_destroy(response);
}

int handshake(t_socket_client* client, t_mensaje *rq) {
	if (rq->type != M_HANDSHAKE) {
		printf("Handshake inválido!\n");
		//log_warning(log, "FSLISTENER", "Handshake invalido");
		return false;
	}
	if (!string_equals_ignore_case((char*) rq->payload, PERSONAJE_HANDSHAKE)) {
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
