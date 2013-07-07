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
#include "../Plataforma.h"
#include "../Planificador/Planificador.h"

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

		switch(mensaje->type){
			case M_HANDSHAKE_PERSONAJE:
				responder_handshake(client);
				break;
			case M_HANDSHAKE_NIVEL:
				responder_handshake(client);
				procesar_handshake_nivel(client);
				break;
			default:
				pthread_mutex_lock(&plataforma->logger_mutex);
				log_warning(plataforma->logger, "Orquestador: Error al recibir el handshake, tipo de mensaje no valido %d", mensaje->type);
				pthread_mutex_unlock(&plataforma->logger_mutex);
				return NULL; //TODO usar send_error_message!!
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
	//char* nivel = (char*) request->payload; TODO no se estaba usando esta variable
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

void procesar_handshake_nivel(t_socket_client* socket_nivel) {

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
		return;
	}

	mensaje = mensaje_deserializer(buffer, 0);
	sockets_bufferDestroy(buffer);

	if (mensaje->type != M_GET_NOMBRE_NIVEL_RESPONSE ) {
		sockets_destroyClient(socket_nivel);
		mensaje_destroy(mensaje);
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_error(plataforma->logger,
				"Orquestador: Tipo de respuesta inválido");
		pthread_mutex_unlock(&plataforma->logger_mutex);
		return;
	}

	if (plataforma_create_nivel(plataforma, mensaje->payload, socket_nivel,
			"127.0.0.1:9000") != 0) { //TODO To-do mal aca! cambiar la ip por la ip real.
		sockets_destroyClient(socket_nivel);
	}
	mensaje_destroy(mensaje);
}

void responder_handshake(t_socket_client* client) {
	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger,
			"Orquestador: Cliente conectado por el socket %d",
			client->socket->desc);
	pthread_mutex_unlock(&plataforma->logger_mutex);
	t_mensaje* mensaje = mensaje_create(M_HANDSHAKE_RESPONSE);
	mensaje_setdata(mensaje, strdup(HANDSHAKE_SUCCESS),
			strlen(HANDSHAKE_SUCCESS) + 1);
	mensaje_send(mensaje, client);
	mensaje_destroy(mensaje);
}

void mostrar_mensaje(t_mensaje* mensaje, t_socket_client* client) {
	printf("Mensaje recibido del socket: %d\n", client->socket->desc);
	printf("TYPE: %d\n", mensaje->type);
	printf("LENGHT: %d\n", mensaje->length);
	printf("PAYLOAD: %s\n", (char*) mensaje->payload);
}
