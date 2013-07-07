#include <stdlib.h>
#include "Planificador.h"
#include "../Plataforma.h"
#include "../Orquestador/Orquestador.h"

void* planificador(void* args) {

	t_plataforma* plataforma = ((thread_planificador_args*) args)->plataforma;
	plataforma_t_nivel* nivel = ((thread_planificador_args*) args)->nivel;

	t_socket_server* server = sockets_createServer(nivel->planificador->ip, nivel->planificador->puerto);

	if (!sockets_listen(server)) {
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_error(plataforma->logger, "Planificador nivel %s: No se puede escuchar", nivel->nombre);
		pthread_mutex_unlock(&plataforma->logger_mutex);
		sockets_destroyServer(server);
	}

	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger,
			"Planificador nivel %s: Escuchando conexiones entrantes en %s:%d",
			nivel->nombre, nivel->planificador->ip, nivel->planificador->puerto);
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
					"Planificador nivel %s: Error al recibir datos en el accept", nivel->nombre);
			pthread_mutex_unlock(&plataforma->logger_mutex);
			return NULL ;
		}

		t_mensaje* mensaje = mensaje_deserializer(buffer, 0);
		sockets_bufferDestroy(buffer);

		//Solo se conectan personajes, asi que solo valido handshake de personajes

		if (mensaje->type != M_HANDSHAKE_PERSONAJE || strcmp(mensaje->payload, PERSONAJE_HANDSHAKE) != 0) {
			mensaje_destroy(mensaje);
			sockets_destroyClient(client);
			return NULL ;
		}
		//TODO necesitamos guardar info del personaje?

		mensaje_destroy(mensaje);
		return client;

	}

	int recvClosure(t_socket_client* client) {
		t_socket_buffer* buffer = sockets_recv(client);

		if (buffer == NULL ) {
			return false;
		}

		t_mensaje* mensaje = mensaje_deserializer(buffer, 0);
		sockets_bufferDestroy(buffer);

		//TODO procesar mensajes que pueden llegar!

		mensaje_destroy(mensaje);
		return true;
	}

	t_list* clients = list_create();

	while (true) {
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_debug(plataforma->logger, "Planificador nivel %s: Entro al select", nivel->nombre);
		pthread_mutex_unlock(&plataforma->logger_mutex);
		sockets_select(servers, clients, 0, &acceptClosure, &recvClosure);
	}

	list_destroy_and_destroy_elements(servers, (void*) sockets_destroyServer);
	list_destroy_and_destroy_elements(clients, (void*) sockets_destroyClient);
	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger, "Planificador nivel %s: Server cerrado correctamente", nivel->nombre);
	pthread_mutex_unlock(&plataforma->logger_mutex);

	return (void*) EXIT_SUCCESS;
}
