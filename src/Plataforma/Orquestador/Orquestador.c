/*
 * Orquestador.c
 *
 *  Created on: 20/06/2013
 *      Author: reyiyo
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <commons/string.h>
#include "Orquestador.h"

#define PUERTO_ORQUESTADOR 5000

void* orquestador(void* plat) {
	t_plataforma* plataforma = (t_plataforma*) plat;
	t_orquestador* self = orquestador_create(PUERTO_ORQUESTADOR, plataforma);

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
		int tipo_mensaje = mensaje->type;
		sockets_bufferDestroy(buffer);
		mensaje_destroy(mensaje);

		switch (tipo_mensaje) {
		case M_HANDSHAKE_PERSONAJE:
			responder_handshake(client, plataforma->logger,
					&plataforma->logger_mutex, "Orquestador");
			break;
		case M_HANDSHAKE_NIVEL:
			responder_handshake(client, plataforma->logger,
					&plataforma->logger_mutex, "Orquestador");
			if (!procesar_handshake_nivel(self, client, plataforma)) {
				return NULL ; //TODO usar send_error_message!!
			}
			break;
		default:
			pthread_mutex_lock(&plataforma->logger_mutex);
			log_warning(plataforma->logger,
					"Orquestador: Error al recibir el handshake, tipo de mensaje no valido %d",
					tipo_mensaje);
			pthread_mutex_unlock(&plataforma->logger_mutex);
			return NULL ; //TODO usar send_error_message!!
		}

		return client;
	}

	int recvClosure(t_socket_client* client) {
		t_socket_buffer* buffer = sockets_recv(client);

		if (buffer == NULL ) {
			return false;
		}

		t_mensaje* mensaje = mensaje_deserializer(buffer, 0);
		mostrar_mensaje(mensaje, client);
		process_request(mensaje, client, plataforma);

		mensaje_destroy(mensaje);
		sockets_bufferDestroy(buffer);
		return true;
	}

	sockets_create_little_server(NULL, self->puerto, plataforma->logger,
			&plataforma->logger_mutex, "Orquestador", self->servers,
			self->clients, &acceptClosure, &recvClosure);

	while (true) {
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_debug(plataforma->logger, "Orquestador: Entro al select");
		pthread_mutex_unlock(&plataforma->logger_mutex);
		sockets_select(self->servers, self->clients, 0, &acceptClosure,
				&recvClosure);
	}

	orquestador_destroy(self);

	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger, "Orquestador: Server cerrado correctamente");
	pthread_mutex_unlock(&plataforma->logger_mutex);

	return (void*) EXIT_SUCCESS;

}

t_orquestador* orquestador_create(int puerto, t_plataforma* plataforma) {
	t_orquestador* new = malloc(sizeof(t_orquestador));
	new->puerto = puerto;
	new->planificadores_count = 0;
	new->clients = list_create();
	new->servers = list_create();
	plataforma->orquestador = new;
	return new;
}

void orquestador_destroy(t_orquestador* self) {
	list_destroy_and_destroy_elements(self->clients,
			(void *) sockets_destroyClient);
	list_destroy_and_destroy_elements(self->servers,
			(void *) sockets_destroyServer);
	free(self);
}

void process_request(t_mensaje* request, t_socket_client* client,
		t_plataforma* plataforma) {
	if (request->type == M_GET_INFO_NIVEL_REQUEST) {
		return orquestador_get_info_nivel(request, client, plataforma);
	} else {
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_warning(plataforma->logger,
				"Orquestador: Tipo de Request desconocido: %d", request->type);
		pthread_mutex_unlock(&plataforma->logger_mutex);
		return orquestador_send_error_message("Request desconocido", client);
	}
}

void orquestador_get_info_nivel(t_mensaje* request, t_socket_client* client,
		t_plataforma* plataforma) {
	char* nivel_pedido = string_duplicate((char*) request->payload);

	plataforma_t_nivel* el_nivel = plataforma_get_nivel(plataforma,
			nivel_pedido);

	if (el_nivel == NULL ) {
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_error(plataforma->logger, "Orquestador: Nivel inválido: %s",
				nivel_pedido);
		pthread_mutex_unlock(&plataforma->logger_mutex);

		free(nivel_pedido);
		orquestador_send_error_message("Nivel inválido", client);
		return;
	}

	t_mensaje* response = mensaje_create(M_GET_INFO_NIVEL_RESPONSE);

	char* nivel_str = string_from_format("%s:%d",
			sockets_getIp(el_nivel->socket_nivel->socket),
			sockets_getPort(el_nivel->socket_nivel->socket));
	t_connection_info* nivel_connection = connection_create(nivel_str);

	t_stream* response_data = get_info_nivel_response_create_serialized(
			nivel_connection, el_nivel->planificador->connection_info);

	mensaje_setdata(response, response_data->data, response_data->length);
	mensaje_send(response, client);
	mensaje_destroy(response);

	free(nivel_str);
	connection_destroy(nivel_connection);
	free(nivel_pedido);
	free(response_data);
}

void orquestador_send_error_message(char* error_description,
		t_socket_client* client) {
	t_mensaje* response = mensaje_create(M_ERROR);
	mensaje_setdata(response, strdup(error_description),
			strlen(error_description) + 1);
	mensaje_send(response, client);
	mensaje_destroy(response);
}

bool procesar_handshake_nivel(t_orquestador* self,
		t_socket_client* socket_nivel, t_plataforma* plataforma) {

	t_mensaje* mensaje = mensaje_create(M_GET_NOMBRE_NIVEL_REQUEST);
	mensaje_send(mensaje, socket_nivel);
	mensaje_destroy(mensaje);

	t_socket_buffer* buffer = sockets_recv(socket_nivel);

	if (buffer == NULL ) {
		sockets_destroyClient(socket_nivel);
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_warning(plataforma->logger,
				"Orquestador: Error al recibir el nombre del nivel");
		pthread_mutex_unlock(&plataforma->logger_mutex);
		return false;
	}

	mensaje = mensaje_deserializer(buffer, 0);
	sockets_bufferDestroy(buffer);

	if (mensaje->type != M_GET_NOMBRE_NIVEL_RESPONSE) {
		sockets_destroyClient(socket_nivel);
		mensaje_destroy(mensaje);
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_error(plataforma->logger,
				"Orquestador: Tipo de respuesta inválido");
		pthread_mutex_unlock(&plataforma->logger_mutex);
		return false;
	}

	char* planificador_connection_info = string_from_format("127.0.0.1:%d",
			9000 + self->planificadores_count);

	if (plataforma_create_nivel(plataforma, mensaje->payload, socket_nivel,
			planificador_connection_info) != 0) {
		self->planificadores_count++;
		mensaje_destroy(mensaje);
		sockets_destroyClient(socket_nivel);

		pthread_mutex_lock(&plataforma->logger_mutex);
		log_error(plataforma->logger,
				"Orquestador: No se pudo crear el planificador para el nivel %s",
				mensaje->payload);
		pthread_mutex_unlock(&plataforma->logger_mutex);

		return false;
	}
	free(planificador_connection_info);

	mensaje_destroy(mensaje);
	return true;
}

void mostrar_mensaje(t_mensaje* mensaje, t_socket_client* client) {
	printf("Mensaje recibido del socket: %d\n", client->socket->desc);
	printf("TYPE: %d\n", mensaje->type);
	printf("LENGHT: %d\n", mensaje->length);
	printf("PAYLOAD: %s\n", (char*) mensaje->payload);
}
