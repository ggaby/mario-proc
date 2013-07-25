#include <stdlib.h>
#include <unistd.h>
#include "Planificador.h"
#include <commons/string.h>
#include <commons/config.h>
#include <commons/temporal.h>
#include "../../common/list.h"

void* planificador(void* args) {
	thread_planificador_args* argumentos = (thread_planificador_args*) args;
	t_plataforma* plataforma = argumentos->plataforma;
	t_planificador* self = planificador_create(plataforma,
			argumentos->planificador_connection_info, argumentos->nombre_nivel);

	free(argumentos->nombre_nivel);
	free(argumentos);

	t_socket_client* acceptClosure(t_socket_server* server) {
		t_socket_client* client = sockets_accept(server);

		t_mensaje* mensaje = mensaje_recibir(client);

		if (mensaje == NULL ) {
			sockets_destroyClient(client);
			pthread_mutex_lock(&plataforma->logger_mutex);
			log_warning(plataforma->logger,
					"%s: Error al recibir datos en el accept", self->log_name);
			pthread_mutex_unlock(&plataforma->logger_mutex);
			return NULL ;
		}

		//Solo se conectan personajes, asi que solo valido handshake de personajes

		if (mensaje->type != M_HANDSHAKE_PERSONAJE
				|| strcmp(mensaje->payload, PERSONAJE_HANDSHAKE) != 0) {
			mensaje_destroy(mensaje);
			sockets_destroyClient(client);
			return NULL ;
		}

		mensaje_destroy(mensaje);

		queue_push(self->personajes_ready,
				planificador_personaje_create(client));

		responder_handshake(client, plataforma->logger,
				&plataforma->logger_mutex, self->log_name);

		if (!planificador_esta_procesando(self)) {
			planificador_mover_personaje(self);
		}

		return client;
	}

	int recvClosure(t_socket_client* client) {
		t_mensaje* mensaje = mensaje_recibir(client);

		if (mensaje == NULL ) {
			pthread_mutex_lock(&plataforma->logger_mutex);
			log_warning(plataforma->logger, "%s: Mensaje recibido NULL.",
					self->log_name);
			pthread_mutex_unlock(&plataforma->logger_mutex);

			verificar_personaje_desconectado(self, plataforma, client);
			return false;
		}

		//TODO iNotify para actualizar el quantum.
		if (!planificador_process_request(self, mensaje, plataforma)) {
			mensaje_destroy(mensaje);
			return false;
		}

		mensaje_destroy(mensaje);
		return true;
	}

	sockets_create_little_server(self->connection_info->ip,
			self->connection_info->puerto, plataforma->logger,
			&plataforma->logger_mutex, self->log_name, self->servers,
			self->clients, &acceptClosure, &recvClosure, NULL );

	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger, "%s: Server cerrado correctamente",
			self->log_name);
	pthread_mutex_unlock(&plataforma->logger_mutex);
	planificador_destroy(self);

	return (void*) EXIT_SUCCESS;
}

t_planificador* planificador_create(t_plataforma* plataforma,
		t_connection_info* connection_info, char* nombre_nivel) {
	t_planificador* new = malloc(sizeof(t_planificador));

	t_config* config = config_create(plataforma->config_path);

	new->log_name = string_from_format("Planificador nivel %s", nombre_nivel);
	new->quantum_total = config_get_int_value(config,
			PLANIFICADOR_CONFIG_QUANTUM);
	new->quantum_restante = new->quantum_total;

	new->tiempo_sleep = config_get_int_value(config,
			PLANIFICADOR_CONFIG_TIEMPO_ESPERA);

	new->personajes_ready = queue_create();
	new->personajes_blocked = queue_create();
	new->personaje_ejecutando = NULL;
	new->servers = list_create();
	new->clients = list_create();
	new->connection_info = connection_info;

	config_destroy(config);

	plataforma_t_nivel* mi_nivel = plataforma_get_nivel(plataforma,
			nombre_nivel);
	mi_nivel->planificador = new;
	return new;
}

void planificador_personaje_destroy(planificador_t_personaje* self) {
	free(self->tiempo_llegada);
	free(self);
}

void planificador_destroy(t_planificador* self) {
	list_destroy_and_destroy_elements(self->clients,
			(void*) sockets_destroyClient);
	list_destroy_and_destroy_elements(self->servers,
			(void*) sockets_destroyServer);
	queue_destroy_and_destroy_elements(self->personajes_ready,
			(void*) planificador_personaje_destroy);
	queue_destroy_and_destroy_elements(self->personajes_blocked,
			(void*) planificador_personaje_destroy);
	if (self->personaje_ejecutando != NULL ) {
		planificador_personaje_destroy(self->personaje_ejecutando);
	}
	connection_destroy(self->connection_info);
	free(self->log_name);
	free(self);
}

planificador_t_personaje* planificador_personaje_create(t_socket_client* socket) {
	planificador_t_personaje* new = malloc(sizeof(planificador_t_personaje));
	new->socket = socket;
	new->tiempo_llegada = temporal_get_string_time();
	return new;
}

bool planificador_esta_procesando(t_planificador* self) {
	return self->personaje_ejecutando != NULL ;
}

void planificador_resetear_quantum(t_planificador* self) {
	self->quantum_restante = self->quantum_total;
}

bool planificador_process_request(t_planificador* self, t_mensaje* mensaje,
		t_plataforma* plataforma) {

	//TODO procesar mensajes que pueden llegar!
	switch (mensaje->type) {
	case M_TURNO_FINALIZADO:
		planificador_finalizar_turno(self);
		break;
	default:
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_warning(plataforma->logger, "%s: error en mensaje recibido tipo %d desconocido.", self->log_name, mensaje->type);
		pthread_mutex_unlock(&plataforma->logger_mutex);
		return false;
	}

	return true;
}

//TODO: Acá habría que ver si quedó bloqueado;
void planificador_finalizar_turno(t_planificador* self) {
	if (self->quantum_restante == 0) {
		planificador_cambiar_de_personaje(self);
		planificador_resetear_quantum(self);
	}

	planificador_mover_personaje(self);
}

/*
 *Envía notificacion de movimiento y consume un Quantum
 */
void planificador_mover_personaje(t_planificador* self) {
	sleep(self->tiempo_sleep);
	self->quantum_restante--;

	if(self->personaje_ejecutando == NULL){
		planificador_cambiar_de_personaje(self);
	}

	t_mensaje* mensaje = mensaje_create(M_NOTIFICACION_MOVIMIENTO);

	mensaje_send(mensaje, self->personaje_ejecutando->socket);

	mensaje_destroy(mensaje);
}

/*
 * Pone el personaje ejecutando al final de la cola de ready y saca el siguiente
 */
void planificador_cambiar_de_personaje(t_planificador* self) {
	if(self->personaje_ejecutando != NULL){
		queue_push(self->personajes_ready, self->personaje_ejecutando);
	}

	self->personaje_ejecutando = queue_pop(self->personajes_ready);
}

void verificar_personaje_desconectado(t_planificador* self,
		t_plataforma* plataforma, t_socket_client* client) {

	bool es_el_personaje(planificador_t_personaje* elem) {
		return sockets_equalsClients(client, elem->socket);
	}

	//TODO: En esta función, en cada caso habría que ver si hay que devolver recursos o algo así

	//Alto hack: queue->elements se puede manejar como una lista y recorrerla ;)
	planificador_t_personaje* personaje_desconectado = list_find(
			self->personajes_ready->elements, (void*) es_el_personaje);
	if (personaje_desconectado != NULL ) {
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_warning(plataforma->logger,
				"%s: El personaje en el socket %d se ha desconectado estando en ready",
				self->log_name, personaje_desconectado->socket->socket->desc);
		pthread_mutex_unlock(&plataforma->logger_mutex);
		my_list_remove_and_destroy_by_condition(
				self->personajes_ready->elements, (void*) es_el_personaje,
				(void*) planificador_personaje_destroy);
	} else {
		personaje_desconectado = list_find(self->personajes_blocked->elements,
				(void*) es_el_personaje);
		if (personaje_desconectado != NULL ) {
			pthread_mutex_lock(&plataforma->logger_mutex);
			log_warning(plataforma->logger,
					"%s: El personaje en el socket %d se ha desconectado estando en blocked",
					self->log_name,
					personaje_desconectado->socket->socket->desc);
			pthread_mutex_unlock(&plataforma->logger_mutex);
			my_list_remove_and_destroy_by_condition(
					self->personajes_blocked->elements, (void*) es_el_personaje,
					(void*) planificador_personaje_destroy);
		} else {
			if (es_el_personaje(self->personaje_ejecutando)) {
				pthread_mutex_lock(&plataforma->logger_mutex);
				log_warning(plataforma->logger,
						"%s: El personaje en el socket %d se ha desconectado estando ejecutando",
						self->log_name,
						personaje_desconectado->socket->socket->desc);
				pthread_mutex_unlock(&plataforma->logger_mutex);
				//TODO: Acá habría que ver de resetear el quantum y devolver recursos
				//planificador_cambiar_de_personaje(self), tal vez?
				planificador_personaje_destroy(self->personaje_ejecutando);
			}
		}
	}

	pthread_mutex_lock(&plataforma->logger_mutex);
	log_debug(plataforma->logger, "%s: Se terminó de limpiar las estructuras",
			self->log_name);
	pthread_mutex_unlock(&plataforma->logger_mutex);
}
