#include <stdlib.h>
#include <unistd.h>
#include "Planificador.h"
#include "../Plataforma.h"
#include "../Orquestador/Orquestador.h"
#include <commons/string.h>
#include <commons/config.h>

void* planificador(void* args) {

	t_plataforma* plataforma = ((thread_planificador_args*) args)->plataforma;
	plataforma_t_nivel* nivel = ((thread_planificador_args*) args)->nivel;

	t_planificador* self = planificador_create(plataforma->config_path);

	t_list *servers = list_create();
	t_list* clients = list_create();
	char* log_name = string_from_format("Planificador nivel %s", nivel->nombre);

	t_socket_client* acceptClosure(t_socket_server* server) {
		t_socket_client* client = sockets_accept(server);
		t_socket_buffer* buffer = sockets_recv(client);

		if (buffer == NULL ) {
			sockets_destroyClient(client);
			pthread_mutex_lock(&plataforma->logger_mutex);
			log_warning(plataforma->logger,
					"Planificador nivel %s: Error al recibir datos en el accept",
					nivel->nombre);
			pthread_mutex_unlock(&plataforma->logger_mutex);
			return NULL ;
		}

		t_mensaje* mensaje = mensaje_deserializer(buffer, 0);
		sockets_bufferDestroy(buffer);

		//Solo se conectan personajes, asi que solo valido handshake de personajes

		if (mensaje->type != M_HANDSHAKE_PERSONAJE
				|| strcmp(mensaje->payload, PERSONAJE_HANDSHAKE) != 0) {
			mensaje_destroy(mensaje);
			sockets_destroyClient(client);
			return NULL ;
		}

		mensaje_destroy(mensaje);

		planificador_t_personaje* personaje = planificador_create_add_personaje(
				self, client);

		responder_handshake(client, plataforma->logger,
				&plataforma->logger_mutex, log_name);

		//TODO ver como inicia el juego!
		if (!planificador_esta_procesando(self)) { //FIXME MMHHH... esto? o ver si la cola de ready esta vacia??
			planificador_mover_personaje(self, personaje);
		}

		return client;

	}

	int recvClosure(t_socket_client* client) {
		t_socket_buffer* buffer = sockets_recv(client);

		if (buffer == NULL ) {
			return false;
		}

		t_mensaje* mensaje = mensaje_deserializer(buffer, 0);
		sockets_bufferDestroy(buffer);

		//TODO iNotify para actualizar el quantum.
		if (!planificador_process_request(self, mensaje)) {
			mensaje_destroy(mensaje);
			return false;
		}

		mensaje_destroy(mensaje);
		return true;
	}

	sockets_create_little_server(nivel->planificador->ip,
			nivel->planificador->puerto, plataforma->logger,
			&plataforma->logger_mutex, log_name, servers, clients,
			&acceptClosure, &recvClosure);

	list_destroy_and_destroy_elements(servers, (void*) sockets_destroyServer);
	list_destroy_and_destroy_elements(clients, (void*) sockets_destroyClient);

	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger, "%s: Server cerrado correctamente", log_name);
	pthread_mutex_unlock(&plataforma->logger_mutex);
	free(log_name);
	planificador_destroy(self);

	return (void*) EXIT_SUCCESS;
}

t_planificador* planificador_create(char* config_path) {
	t_planificador* new = malloc(sizeof(t_planificador));

	t_config* config = config_create(config_path);

	new->quantum_total = config_get_int_value(config,
			PLANIFICADOR_CONFIG_QUANTUM);
	new->quantum_actual = new->quantum_total;

	new->tiempo_entre_movimientos = config_get_int_value(config,
			PLANIFICADOR_CONFIG_TIEMPO_ESPERA);

	new->personajes_ready = queue_create();
	new->personajes_blocked = queue_create();

	config_destroy(config);
	return new;
}

void planificador_personaje_destroy(planificador_t_personaje* personaje) {
	sockets_destroyClient(personaje->socket);
	free(personaje);
}

void planificador_destroy(t_planificador* self) {
	queue_destroy_and_destroy_elements(self->personajes_ready,
			(void*) planificador_personaje_destroy);
	queue_destroy_and_destroy_elements(self->personajes_blocked,
			(void*) planificador_personaje_destroy);
	free(self);
}

/*
 * Crea el personaje, lo mete en la cola de ready y retorna ese personaje.
 */
planificador_t_personaje* planificador_create_add_personaje(
		t_planificador* self, t_socket_client* socket) {
	planificador_t_personaje* new_personaje = malloc(
			sizeof(planificador_t_personaje));
	new_personaje->socket = socket;
	queue_push(self->personajes_ready, new_personaje);
	//No hace falta hacer free de new_personaje porque queda referenciado por la cola.

	return new_personaje;
}

bool planificador_esta_procesando(t_planificador* self) {
	return self->quantum_total != self->quantum_actual;
}

void planificador_resetear_quantum(t_planificador* self) {
	self->quantum_actual = self->quantum_total;
}

bool planificador_process_request(t_planificador* self, t_mensaje* mensaje) {

	//TODO procesar mensajes que pueden llegar!
	switch (mensaje->type) {
	case M_TURNO_FINALIZADO:
		planificador_finalizar_turno(self);
		break;
	default:
		//TODO como llega la plataforma aca? lo recibo de parametro o planificador conoce al mismo log que tiene la plataforma?
//			log_warning(plataforma->logger, "Planificador nivel..: error en mensaje recibido");
		return false;
	}

	return true;
}

void planificador_finalizar_turno(t_planificador* self) {
	if (self->quantum_actual == 0) {
		planificador_cambiar_de_personaje(self);
		planificador_resetear_quantum(self);
	}

	planificador_t_personaje* personaje = queue_peek(self->personajes_ready);
	planificador_mover_personaje(self, personaje);
}

/*
 *Envia notificacion de movimiento y consume un Quantum
 */
void planificador_mover_personaje(t_planificador* self,
		planificador_t_personaje* personaje) {
	sleep(self->tiempo_entre_movimientos);
	self->quantum_actual--;

	t_mensaje* mensaje = mensaje_create(M_NOTIFICACION_MOVIMIENTO);
	//TODO SETEAR DATA... QUE DATA??

	mensaje_send(mensaje, personaje->socket);
}

/*
 * Pone el primer personaje de ready al final
 */
void planificador_cambiar_de_personaje(t_planificador* self) {
	planificador_t_personaje* personaje = queue_pop(self->personajes_ready);
	queue_push(self->personajes_ready, personaje);
}
