#include <stdlib.h>
#include <unistd.h>
#include "Planificador.h"
#include <commons/string.h>
#include <commons/config.h>
#include <commons/temporal.h>
#include "../../common/list.h"
#include <sys/inotify.h>

#define INOTIFY_EVENT_SIZE  ( sizeof (struct inotify_event) + 50 )
#define INOTIFY_BUF_LEN     ( 1024 * INOTIFY_EVENT_SIZE )

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

		if (mensaje == NULL) {
			sockets_destroyClient(client);
			pthread_mutex_lock(&plataforma->logger_mutex);
			log_warning(plataforma->logger,
					"%s: Error al recibir datos en el accept", self->log_name);
			pthread_mutex_unlock(&plataforma->logger_mutex);
			return NULL;
		}

		//Solo se conectan personajes, asi que solo valido handshake de personajes

		if (mensaje->type != M_HANDSHAKE_PERSONAJE
				|| strcmp(mensaje->payload, PERSONAJE_HANDSHAKE) != 0) {
			mensaje_destroy(mensaje);
			sockets_destroyClient(client);
			return NULL;
		}

		mensaje_destroy(mensaje);

		responder_handshake(client, plataforma->logger,
				&plataforma->logger_mutex, self->log_name);

		queue_push(self->personajes_ready,
				planificador_personaje_create(client, plataforma));

		if (!planificador_esta_procesando(self)) {
			planificador_mover_personaje(self);
		}

		return client;
	}

	int recvClosure(t_socket_client* client) {

		bool es_el_fd_de_inotify(t_socket_client* client) {
			return sockets_equalsClients(client, self->inotify_fd_wrapper);
		}

		if (es_el_fd_de_inotify(client)) {
			planificador_reload_config(self, client, plataforma);
			return true;
		} else {

			t_mensaje* mensaje = mensaje_recibir(client);

			if (mensaje == NULL) {
				pthread_mutex_lock(&plataforma->logger_mutex);
				log_warning(plataforma->logger, "%s: Mensaje recibido NULL.",
						self->log_name);
				pthread_mutex_unlock(&plataforma->logger_mutex);

				verificar_personaje_desconectado(self, plataforma, client);
				return false;
			}

			if (!planificador_process_request(self, mensaje, plataforma)) {
				mensaje_destroy(mensaje);
				return false;
			}

			mensaje_destroy(mensaje);
			return true;
		}
	}

	sockets_create_little_server(self->connection_info->ip,
			self->connection_info->puerto, plataforma->logger,
			&plataforma->logger_mutex, self->log_name, self->servers,
			self->clients, &acceptClosure, &recvClosure, NULL);

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

	new->tiempo_sleep = config_get_double_value(config,
	PLANIFICADOR_CONFIG_TIEMPO_ESPERA) * 1000000;

	new->personajes_ready = queue_create();
	new->personajes_blocked = dictionary_create();
	new->personaje_ejecutando = NULL;
	new->servers = list_create();
	new->clients = list_create();
	new->connection_info = connection_info;

	config_destroy(config);

	plataforma_t_nivel* mi_nivel = plataforma_get_nivel_by_nombre(plataforma,
			nombre_nivel);
	mi_nivel->planificador = new;

	new->inotify_fd_wrapper = inotify_socket_wrapper_create(
			plataforma->config_path);
	list_add(new->clients, new->inotify_fd_wrapper);

	pthread_mutex_init(&new->planificador_mutex, NULL);
	return new;
}

void planificador_destroy(t_planificador* self) {
	list_destroy_and_destroy_elements(self->clients,
			(void*) sockets_destroyClient);
	list_destroy_and_destroy_elements(self->servers,
			(void*) sockets_destroyServer);
	queue_destroy_and_destroy_elements(self->personajes_ready,
			(void*) planificador_personaje_destroy);

	// IF YOU KNOW WHAT I MEAN ;)
	void destruir_la_cola(char* key, t_queue* queue) {
		queue_destroy_and_destroy_elements(queue,
				(void*) planificador_personaje_destroy);
	}

	dictionary_iterator(self->personajes_blocked, (void*) destruir_la_cola);
	dictionary_destroy(self->personajes_blocked);

	if (self->personaje_ejecutando != NULL) {
		planificador_personaje_destroy(self->personaje_ejecutando);
	}
	connection_destroy(self->connection_info);
	free(self->log_name);
	pthread_mutex_destroy(&self->planificador_mutex);
	free(self);
}

void planificador_personaje_destroy(planificador_t_personaje* self) {
	free(self->tiempo_llegada);
	free(self);
}

planificador_t_personaje* planificador_personaje_create(t_socket_client* socket,
		t_plataforma* plataforma) {

	planificador_t_personaje* new = malloc(sizeof(planificador_t_personaje));
	char* id = mensaje_get_simbolo_personaje(socket, plataforma->logger,
			&plataforma->logger_mutex);
	new->id = id[0];
	free(id);

	new->socket = socket;
	new->tiempo_llegada = temporal_get_string_time();

	return new;

}

bool planificador_esta_procesando(t_planificador* self) {
	return self->personaje_ejecutando != NULL;
}

void planificador_resetear_quantum(t_planificador* self) {
	self->quantum_restante = self->quantum_total;
}

bool planificador_process_request(t_planificador* self, t_mensaje* mensaje,
		t_plataforma* plataforma) {

	switch (mensaje->type) {
	case M_TURNO_FINALIZADO_OK:
		planificador_finalizar_turno(self);
		break;
	case M_TURNO_FINALIZADO_SOLICITANDO_RECURSO:
		self->quantum_restante = 0;
		planificador_finalizar_turno(self);
		break;
	case M_TURNO_FINALIZADO_BLOCKED:
		planificador_finalizar_turno_bloqueado(self, mensaje);
		break;
	default:
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_warning(plataforma->logger,
				"%s: error en mensaje recibido tipo %d desconocido.",
				self->log_name, mensaje->type);
		pthread_mutex_unlock(&plataforma->logger_mutex);
		return false;
	}

	return true;
}

void planificador_finalizar_turno(t_planificador* self) {
	pthread_mutex_lock(&self->planificador_mutex);
	if (self->quantum_restante == 0) {
		planificador_cambiar_de_personaje(self);
	}

	planificador_mover_personaje(self);
	pthread_mutex_unlock(&self->planificador_mutex);
}

void planificador_finalizar_turno_bloqueado(t_planificador* self,
		t_mensaje* mensaje) {
	pthread_mutex_lock(&self->planificador_mutex);
	bloquear_personaje(self, (char*) mensaje->payload);
	planificador_mover_personaje(self);
	pthread_mutex_unlock(&self->planificador_mutex);
}

void bloquear_personaje(t_planificador* self, char* recurso) {
	if (!dictionary_has_key(self->personajes_blocked, recurso)) {
		dictionary_put(self->personajes_blocked, recurso, queue_create());
	}
	t_queue* cola_bloqueados = dictionary_get(self->personajes_blocked,
			recurso);
	queue_push(cola_bloqueados, self->personaje_ejecutando);
	self->personaje_ejecutando = queue_pop(self->personajes_ready);
	planificador_resetear_quantum(self);
}

/*
 *Envía notificacion de movimiento y consume un Quantum
 */
void planificador_mover_personaje(t_planificador* self) {
	usleep(self->tiempo_sleep);

	if (self->personaje_ejecutando == NULL || self->quantum_restante == 0) {
		planificador_cambiar_de_personaje(self);
	}

	if (self->personaje_ejecutando == NULL) { //No hay más personajes (chanchada.com)
		return;
	}

	self->quantum_restante--;

	t_mensaje* mensaje = mensaje_create(M_NOTIFICACION_MOVIMIENTO);

	mensaje_send(mensaje, self->personaje_ejecutando->socket);

	mensaje_destroy(mensaje);
}

/*
 * Pone el personaje ejecutando al final de la cola de ready y saca el siguiente
 */
void planificador_cambiar_de_personaje(t_planificador* self) {
	if (self->personaje_ejecutando != NULL) {
		queue_push(self->personajes_ready, self->personaje_ejecutando);
	}

	self->personaje_ejecutando = queue_pop(self->personajes_ready);
	planificador_resetear_quantum(self);
}

void verificar_personaje_desconectado(t_planificador* self,
		t_plataforma* plataforma, t_socket_client* client) {
	pthread_mutex_lock(&self->planificador_mutex);
	bool es_el_personaje(planificador_t_personaje* elem) {
		return sockets_equalsClients(client, elem->socket);
	}

//Alto hack: queue->elements se puede manejar como una lista y recorrerla ;)
	planificador_t_personaje* personaje_desconectado = list_find(
			self->personajes_ready->elements, (void*) es_el_personaje);
	if (personaje_desconectado != NULL) {
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_warning(plataforma->logger,
				"%s: El personaje en el socket %d se ha desconectado estando en ready",
				self->log_name, personaje_desconectado->socket->socket->desc);
		pthread_mutex_unlock(&plataforma->logger_mutex);
		my_list_remove_and_destroy_by_condition(
				self->personajes_ready->elements, (void*) es_el_personaje,
				(void*) planificador_personaje_destroy);
	} else {
		personaje_desconectado = buscar_personaje_bloqueado_by_socket(self,
				client);
		if (personaje_desconectado != NULL) {
			pthread_mutex_lock(&plataforma->logger_mutex);
			log_warning(plataforma->logger,
					"%s: El personaje en el socket %d se ha desconectado estando en blocked",
					self->log_name,
					personaje_desconectado->socket->socket->desc);
			pthread_mutex_unlock(&plataforma->logger_mutex);

			remover_de_bloqueados(self, personaje_desconectado);

		} else {
			if (es_el_personaje(self->personaje_ejecutando)) {
				pthread_mutex_lock(&plataforma->logger_mutex);
				log_warning(plataforma->logger,
						"%s: El personaje en el socket %d se ha desconectado estando ejecutando",
						self->log_name,
						self->personaje_ejecutando->socket->socket->desc);
				pthread_mutex_unlock(&plataforma->logger_mutex);
				//TODO: Acá habría que ver de devolver recursos
				planificador_personaje_destroy(self->personaje_ejecutando);
				self->personaje_ejecutando = NULL;
				planificador_cambiar_de_personaje(self);
				planificador_mover_personaje(self);
			}
		}
	}

	pthread_mutex_lock(&plataforma->logger_mutex);
	log_debug(plataforma->logger, "%s: Se terminó de limpiar las estructuras",
			self->log_name);
	pthread_mutex_unlock(&plataforma->logger_mutex);
	pthread_mutex_unlock(&self->planificador_mutex);
}

planificador_t_personaje* buscar_personaje_bloqueado_by_socket(
		t_planificador* self, t_socket_client* client) {

	bool tiene_el_mismo_socket(planificador_t_personaje* elem) {
		return sockets_equalsClients(client, elem->socket);
	}

	return buscar_personaje_bloqueado(self, (void*) tiene_el_mismo_socket);
}

planificador_t_personaje* buscar_personaje_bloqueado_by_id(t_planificador* self,
		char id) {

	bool tiene_el_mismo_id(planificador_t_personaje* elem) {
		return elem->id == id;
	}

	return buscar_personaje_bloqueado(self, (void*) tiene_el_mismo_id);
}

planificador_t_personaje* buscar_personaje_bloqueado(t_planificador* self,
bool condision(void*)) {
	t_list* colas = list_create();

	//If you know what I mean ;)
	void get_cola(char* key, t_queue* elem) {
		list_add(colas, elem);
	}

	dictionary_iterator(self->personajes_blocked, (void*) get_cola);

	bool buscar_la_cola(t_queue* cola) {
		planificador_t_personaje* el_personaje = list_find(cola->elements,
				(void*) condision);
		return el_personaje != NULL;
	}

	t_queue* la_cola = list_find(colas, (void*) buscar_la_cola);

	planificador_t_personaje* el_personaje = NULL;
	if (la_cola != NULL) {
		el_personaje = list_find(la_cola->elements, (void*) condision);
	}

	free(colas); //Colas gratis o liberen las colas

	return el_personaje;
}

void remover_de_bloqueados(t_planificador* self,
		planificador_t_personaje* personaje) {

	t_list* colas = list_create();
	void get_cola(char* key, t_queue* elem) {
		list_add(colas, elem);
	}

	dictionary_iterator(self->personajes_blocked, (void*) get_cola);

	bool es_el_personaje(planificador_t_personaje* elem) {
		return sockets_equalsClients(personaje->socket, elem->socket);
	}

	bool buscar_la_cola(t_queue* cola) {
		planificador_t_personaje* el_personaje = list_find(cola->elements,
				(void*) es_el_personaje);
		return el_personaje != NULL;
	}

	t_queue* la_cola = list_find(colas, (void*) buscar_la_cola);

	my_list_remove_and_destroy_by_condition(la_cola->elements,
			(void*) es_el_personaje, (void*) planificador_personaje_destroy);

	free(colas); //Colas gratis o liberen las colas
}

t_socket_client* inotify_socket_wrapper_create(char* file_to_watch) {
	t_socket_client* client = malloc(sizeof(t_socket_client));
	client->socket = malloc(sizeof(t_socket));
	client->socket->desc = inotify_init();
	if (client->socket->desc < 0) {
		free(client->socket);
		free(client);
		return NULL;
	}
	inotify_add_watch(client->socket->desc, file_to_watch,
	IN_MODIFY | IN_CREATE);
	client->socket->my_addr = malloc(sizeof(struct sockaddr_in));
	sockets_setState(client, SOCKETSTATE_CONNECTED);
	return client;
}

void planificador_reload_config(t_planificador* self, t_socket_client* client,
		t_plataforma* plataforma) {
	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger,
			"%s: Se ha detectado un cambio en el archivo de configuración. Recargando...",
			self->log_name);
	pthread_mutex_unlock(&plataforma->logger_mutex);
	char buffer[INOTIFY_BUF_LEN];
	int length = read(client->socket->desc, buffer, INOTIFY_BUF_LEN);
	if (length < 0) {
		return;
	}
	struct inotify_event *event = (struct inotify_event *) &buffer;
	if (event->mask & (IN_CREATE | IN_MODIFY)) {
		t_config* config = config_create(plataforma->config_path);
		self->quantum_total = config_get_int_value(config,
		PLANIFICADOR_CONFIG_QUANTUM);
		self->tiempo_sleep = config_get_double_value(config,
		PLANIFICADOR_CONFIG_TIEMPO_ESPERA) * 1000000;
		config_destroy(config);
	}

	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger,
			"%s: Nuevo Quantum: %d, Nuevo tiempo de sleep: %d", self->log_name,
			self->quantum_total, self->tiempo_sleep);
	pthread_mutex_unlock(&plataforma->logger_mutex);
}

planificador_t_personaje* planificador_recurso_liberado(
		t_plataforma* plataforma, t_planificador* self, char simbolo) {

	pthread_mutex_lock(&plataforma->logger_mutex);
	log_debug(plataforma->logger, "%s: Recurso para liberar: %c",
			self->log_name, simbolo);
	pthread_mutex_unlock(&plataforma->logger_mutex);

	char key[2] = "";
	key[0] = simbolo;
	key[1] = '\0';

	t_queue* cola_bloqueados = dictionary_get(self->personajes_blocked, key);

	if (cola_bloqueados != NULL) {
		planificador_t_personaje* personaje_desbloqueado = queue_pop(
				cola_bloqueados);

		if (personaje_desbloqueado != NULL) {
			queue_push(self->personajes_ready, personaje_desbloqueado);

			pthread_mutex_lock(&plataforma->logger_mutex);
			log_debug(plataforma->logger, "%s: Personaje liberado: %c",
					self->log_name, personaje_desbloqueado->id);
			pthread_mutex_unlock(&plataforma->logger_mutex);

			if (!planificador_esta_procesando(self)) {
				planificador_mover_personaje(self);
			}
		}

		return personaje_desbloqueado;
	}
	return NULL;
}
