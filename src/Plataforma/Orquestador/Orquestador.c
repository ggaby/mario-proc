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
#include "../Plataforma.h"

#define PUERTO_ORQUESTADOR 5000

t_plataforma* plataforma;

void* orquestador(void* plat) {
	plataforma = (t_plataforma*) plat;
	t_socket_server* server = sockets_createServer(NULL, PUERTO_ORQUESTADOR);

	if (!sockets_listen(server)) {

		pthread_mutex_lock(&plataforma->logger_mutex);
		log_error(plataforma->logger, "Orquestador: No se puede escuchar");
		pthread_mutex_unlock(&plataforma->logger_mutex);
		sockets_destroyServer(server);
	}

	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger,
			"Orquestador: Escuchando conexiones entrantes en 127.0.0.1:%d",
			PUERTO_ORQUESTADOR);
	pthread_mutex_unlock(&plataforma->logger_mutex);

	t_list *servers = list_create();
	list_add(servers, server);

	t_socket_client* acceptClosure(t_socket_server* server) {
		t_socket_client* client = sockets_accept(server);
		t_socket_buffer* buffer = sockets_recv(client);
		if (buffer == NULL ) {
			sockets_destroyClient(client);
			pthread_mutex_lock(&plataforma->logger_mutex);
			log_warning(plataforma->logger,
					"Orquestador: Error al recibir datos en el accept");
			pthread_mutex_unlock(&plataforma->logger_mutex);
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
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_debug(plataforma->logger, "Orquestador: Entro al select");
		pthread_mutex_unlock(&plataforma->logger_mutex);
		sockets_select(servers, clients, 0, &acceptClosure, &recvClosure);
	}

	list_destroy_and_destroy_elements(servers, (void*) sockets_destroyServer);
	list_destroy_and_destroy_elements(clients, (void*) sockets_destroyClient);
	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger, "Orquestador: Server cerrado correctamente");
	pthread_mutex_unlock(&plataforma->logger_mutex);
	return (void*) EXIT_SUCCESS;

}

void process_request(t_mensaje* request, t_socket_client* client) {
	if (request->type == M_GET_INFO_NIVEL_REQUEST) {
		return orquestador_get_info_nivel(request, client);
	} else {
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_warning(plataforma->logger,
				"Orquestador: Tipo de Request desconocido: %d", request->type);
		pthread_mutex_unlock(&plataforma->logger_mutex);
		return orquestador_send_error_message("Request desconocido", client);
	}
}

void orquestador_get_info_nivel(t_mensaje* request, t_socket_client* client) {
	char* nivel = (char*) request->payload;
	t_mensaje* response = mensaje_create(M_GET_INFO_NIVEL_RESPONSE);
	mensaje_setdata(response, string_duplicate("213.456.789.123:8080"),
			strlen("213.456.789.123:8080") + 1);
	mensaje_send(response, client);
	mensaje_destroy(response);
}

void orquestador_send_error_message(char* error_description,
		t_socket_client* client) {
	t_mensaje* response = mensaje_create(M_ERROR);
	mensaje_setdata(response, strdup(error_description),
			strlen(error_description) + 1);
	mensaje_send(response, client);
	mensaje_destroy(response);
}

int handshake(t_socket_client* client, t_mensaje *rq) {
	if (rq->type != M_HANDSHAKE) {
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_warning(plataforma->logger, "Orquestador: Handshake inválido!");
		pthread_mutex_unlock(&plataforma->logger_mutex);
		return false;
	}
	if (!string_equals_ignore_case((char*) rq->payload, PERSONAJE_HANDSHAKE)) {
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_warning(plataforma->logger, "Orquestador: Handshake inválido!");
		pthread_mutex_unlock(&plataforma->logger_mutex);
		return false;
	}
	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger,
			"Orquestador: Cliente conectado por el socket %d",
			client->socket->desc);
	pthread_mutex_unlock(&plataforma->logger_mutex);
	t_mensaje* mensaje = mensaje_create(M_HANDSHAKE);
	mensaje_setdata(mensaje, strdup(HANDSHAKE_SUCCESS),
			strlen(HANDSHAKE_SUCCESS) + 1);
	mensaje_send(mensaje, client);
	mensaje_destroy(mensaje);
	return true;
}

void mostrar_mensaje(t_mensaje* mensaje, t_socket_client* client) {
	printf("Mensaje recibido del socket: %d\n", client->socket->desc);
	printf("TYPE: %d\n", mensaje->type);
	printf("LENGHT: %d\n", mensaje->length);
	printf("PAYLOAD: %s\n", (char*) mensaje->payload);
}
