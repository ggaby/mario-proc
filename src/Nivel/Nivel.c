#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "Nivel.h"
#include "../common/list.h"
#include <commons/string.h>
#include <commons/collections/list.h>
#include "../common/common_structs.h"
#include "../common/list.h"
#include "VerificadorDeadlock/verificador_deadlock.h"

//Cambiar por el puerto del archivo de configuracion correspondiente
#define PORT 5001

bool verificar_argumentos(int argc, char* argv[]);
nivel_t_personaje* nivel_create_personaje(nivel_t_nivel* self,
		char id_personaje, t_socket_client* socket);
void cargar_recursos_config(t_config* config, nivel_t_nivel* new);
void nivel_create_grafica_recurso(nivel_t_nivel* self, t_recurso* recurso);
t_recurso* nivel_create_recurso(char* config_string);
void nivel_destroy_personaje(nivel_t_personaje* personaje);
void nivel_destroy_recurso(t_recurso* recurso);
void nivel_mover_personaje(nivel_t_nivel* self, t_posicion* posicion,
		t_socket_client* client);

int main(int argc, char *argv[]) {

	if (!verificar_argumentos(argc, argv)) {
		printf("Argumentos inválidos!\n");
		return EXIT_FAILURE;
	}

	nivel_t_nivel* self = nivel_create(argv[1]);

	if (!nivel_conectar_a_orquestador(self)) {
		nivel_destroy(self);
		return EXIT_FAILURE;
	}

	list_add(self->clients, self->socket_orquestador);

	t_socket_client* acceptClosure(t_socket_server* server) {
		t_socket_client* client = sockets_accept(server);

		t_mensaje* mensaje = mensaje_recibir(client);

		if (mensaje == NULL ) {
			sockets_destroyClient(client);
			nivel_loguear(log_warning, self,
					"Error al recibir datos en el accept");
			return NULL ;
		}

		if (mensaje->type != M_HANDSHAKE_PERSONAJE
				|| strcmp(mensaje->payload, PERSONAJE_HANDSHAKE) != 0) {
			mensaje_destroy(mensaje);
			sockets_destroyClient(client);
			nivel_loguear(log_warning, self, "Handshake recibido invalido.");
			return NULL ;
		}

		mensaje_destroy(mensaje);

		responder_handshake(client, self->logger, &self->logger_mutex,
				string_from_format("Nivel %s", self->nombre));

		mensaje = mensaje_create(M_GET_SYMBOL_PERSONAJE_REQUEST);
		mensaje_send(mensaje, client);
		mensaje_destroy(mensaje);

		mensaje = mensaje_recibir(client);

		if (mensaje->type != M_GET_SYMBOL_PERSONAJE_RESPONSE) {
			mensaje_destroy(mensaje);
			sockets_destroyClient(client);
			nivel_loguear(log_warning, self,
					"Error recibiendo data del personaje.");
			return NULL ;
		}

		list_add(self->personajes,
				nivel_create_personaje(self, ((char*) mensaje->payload)[0],
						client));
		mensaje_destroy(mensaje);

		return client;
	}

	//closure que maneja los mensajes recibidos
	int recvClosure(t_socket_client* client) {

		t_mensaje* mensaje = mensaje_recibir(client);

		if (mensaje == NULL ) {
			nivel_loguear(log_warning, self, "Mensaje recibido NULL.");
			verificar_personaje_desconectado(self, client);
			return false;
		}

		//TODO hacer un case, por cada tipo de mensaje que manden los personajes y el orquestador_info

		switch (mensaje->type) {
		case M_GET_NOMBRE_NIVEL_REQUEST:
			nivel_get_nombre(self, client);
			break;
		case M_GET_POSICION_RECURSO_REQUEST:
			nivel_get_posicion_recurso(self, mensaje->payload, client);
			break;
		case M_SOLICITUD_MOVIMIENTO_REQUEST:
			nivel_mover_personaje(self, posicion_duplicate(mensaje->payload),
					client);
			break;
		case M_SOLICITUD_RECURSO_REQUEST:
			//TODO verificar si el personaje se encuentra en la posicion del recurso recibido y asignar una instancia.
			break;
		default:
			nivel_loguear(log_warning, self,
					"Tipo del mensaje recibido no valido tipo: %d",
					mensaje->type);
			return false; //TODO WTF this FALSE!! hacer un send_error_message
		}

		mensaje_destroy(mensaje);
		return true;
	}

	void onSelectClosure() {
		mapa_dibujar(self->mapa);
	}

	nivel_create_verificador_deadlock(self);

	sockets_create_little_server(NULL, self->puerto, self->logger, &self->logger_mutex,
			self->nombre, self->servers, self->clients, &acceptClosure,
			&recvClosure, &onSelectClosure);

	nivel_loguear(log_info, self, "cerrado correctamente");
	nivel_destroy(self);

	return EXIT_SUCCESS;
}

nivel_t_nivel* nivel_create(char* config_path) {
	nivel_t_nivel* new = malloc(sizeof(nivel_t_nivel));
	t_config* config = config_create(config_path);
	new->nombre = string_duplicate(config_get_string_value(config, "Nombre"));
	new->tiempoChequeoDeadlock = config_get_int_value(config,
			"TiempoChequeoDeadlock");
	new->recovery = config_get_int_value(config, "Recovery");

	new->orquestador_info = connection_create(
			config_get_string_value(config, "orquestador"));

	new->socket_orquestador = NULL;
	new->clients = list_create();
	new->servers = list_create();
	new->personajes = list_create();
	new->recursos = list_create();

	char* log_file = string_duplicate("nivel.log");
	char* log_level = string_duplicate("INFO");

	if (config_has_property(config, "logFile")) {
		log_file = string_duplicate(config_get_string_value(config, "logFile"));
	}
	if (config_has_property(config, "logLevel")) {
		log_level = string_duplicate(
				config_get_string_value(config, "logLevel"));
	}

	new->logger = log_create(log_file, "Nivel", false,
			log_level_from_string(log_level));

	pthread_mutex_init(&new->logger_mutex, NULL );

	free(log_file);
	free(log_level);

	if (!config_has_property(config, "puerto")) {
		config_destroy(config);
		log_error(new->logger,
				"Error en archivo de configuracion: falta el puerto.");
		nivel_destroy(new);
		return NULL ;
	}

	new->puerto = config_get_int_value(config, "puerto");

	int cols = config_get_int_value(config, NIVEL_CONFIG_COLUMNAS);
	int rows = config_get_int_value(config, NIVEL_CONFIG_FILAS);

	new->mapa = mapa_create(rows, cols);

	cargar_recursos_config(config, new);

	config_destroy(config);

	log_info(new->logger, "Nivel  %s: creado correctamente", new->nombre);
	return new;
}

void nivel_destroy(nivel_t_nivel* self) {
	free(self->nombre);
	connection_destroy(self->orquestador_info);
	log_destroy(self->logger);
	pthread_mutex_destroy(&self->logger_mutex);

	//FIXME: Para qué vendría a ser esto?
//	if (self->socket_orquestador != NULL ) {
//		bool is_socket_orquestador(t_socket_client* elem) {
//			return sockets_equalsClients(self->socket_orquestador, elem);
//		}
//
//		my_list_remove_and_destroy_by_condition(self->clients,
//				(void*) is_socket_orquestador, (void*) sockets_destroyClient);
//	}

	mapa_destroy(self->mapa);

	list_destroy_and_destroy_elements(self->personajes,
			(void*) nivel_destroy_personaje);
	list_destroy_and_destroy_elements(self->recursos,
			(void*) nivel_destroy_recurso);

	list_destroy_and_destroy_elements(self->clients,
			(void*) sockets_destroyClient);
	list_destroy_and_destroy_elements(self->servers,
			(void*) sockets_destroyServer);

	free(self);
}

void nivel_destroy_personaje(nivel_t_personaje* personaje) {
	//sockets_destroyClient(personaje->socket);
	posicion_destroy(personaje->posicion);
	list_destroy_and_destroy_elements(personaje->recursos_asignados,
			(void*) nivel_destroy_recurso);
	free(personaje);
}

void nivel_destroy_recurso(t_recurso* recurso) {
	free(recurso->nombre);
	posicion_destroy(recurso->posicion);
	free(recurso);
}

void nivel_get_nombre(nivel_t_nivel* self, t_socket_client* client) {
	mensaje_create_and_send(M_GET_NOMBRE_NIVEL_RESPONSE,
			string_duplicate(self->nombre), strlen(self->nombre) + 1, client);
}

void nivel_loguear(void log_fn(t_log*, const char*, ...), nivel_t_nivel* self,
		const char* message, ...) {
	pthread_mutex_lock(&self->logger_mutex);
	va_list arguments;
	va_start(arguments, message);
	char* template = string_from_format("Nivel %s: %s", self->nombre, message);
	log_fn(self->logger, string_from_vformat(template, arguments));
	free(template);
	va_end(arguments);
	pthread_mutex_unlock(&self->logger_mutex);
}

void nivel_get_posicion_recurso(nivel_t_nivel* self, char* id_recurso,
		t_socket_client* client) {

	nivel_loguear(log_info, self, "solicitud posicion recurso %s recibida",
			id_recurso);

	bool es_el_recurso(t_recurso* recurso) {
		return id_recurso[0] == recurso->simbolo;
	}

	t_recurso* recurso = list_find(self->recursos, (void*) es_el_recurso);

	if (recurso == NULL ) {
		char* mensaje_error = string_from_format(
				"el recurso %s no se encuentra en el nivel %s", id_recurso,
				self->nombre);

		nivel_loguear(log_warning, self, mensaje_error);

		mensaje_create_and_send(M_ERROR, string_duplicate(mensaje_error),
				strlen(mensaje_error), client);

		free(mensaje_error);
		return;
	}

	nivel_loguear(log_info, self,
			"enviando posicion recurso %s, posicion (%d,%d)", id_recurso,
			recurso->posicion->x, recurso->posicion->y);

	mensaje_create_and_send(M_GET_POSICION_RECURSO_RESPONSE,
			posicion_duplicate(recurso->posicion), sizeof(t_posicion), client);

}

bool verificar_argumentos(int argc, char* argv[]) {
//TODO Validar parametros: ver si no es igual al de personaje para no repetir codigo. Tal vez se puede parametrizar la cantidad de args.
	if (argc < 2) {
		printf("Error en la cantidad de argumentos.\n");
		return false;
	}
	return true;
}

bool nivel_conectar_a_orquestador(nivel_t_nivel* self) {
	//TODO a esta altura no se encuentra creado el thread de verificador_deadlock es necesario hacer toodo el refactor para usar el semaforo de log? :S
	self->socket_orquestador = sockets_conectar_a_servidor(NULL, self->puerto,
			self->orquestador_info->ip, self->orquestador_info->puerto,
			self->logger, M_HANDSHAKE_NIVEL, NIVEL_HANDSHAKE, HANDSHAKE_SUCCESS,
			"Orquestador");

	return (self->socket_orquestador != NULL );
}

nivel_t_personaje* nivel_create_personaje(nivel_t_nivel* self,
		char id_personaje, t_socket_client* socket) {
	nivel_t_personaje* personaje = malloc(sizeof(nivel_t_personaje));

	personaje->id = id_personaje;
	personaje->posicion = posicion_create(0, 0);
	personaje->socket = socket;
	personaje->recursos_asignados = list_create();

	mapa_create_personaje(self->mapa, id_personaje);
	return personaje;
}

void nivel_mover_personaje(nivel_t_nivel* self, t_posicion* posicion,
		t_socket_client* client) {

	bool is_personaje(nivel_t_personaje* personaje) {
		return sockets_equalsClients(personaje->socket, client);
	}

	nivel_t_personaje* personaje = list_find(self->personajes,
			(void*) is_personaje);

	int distancia = posicion_get_distancia(personaje->posicion, posicion);

	if (distancia != DISTANCIA_MOVIMIENTO_PERMITIDA) {
		char* mensaje_error =
				string_from_format(
						"Se quiere mover desde (%d,%d) a (%d,%d): distancia %d (distancia permitida 1)",
						personaje->posicion->x, personaje->posicion->y,
						posicion->x, posicion->y, distancia);

		mensaje_create_and_send(M_ERROR, string_duplicate(mensaje_error),
				strlen(mensaje_error), client);

		free(mensaje_error);
		return;
	}

	if (!mapa_mover_personaje(self->mapa, personaje->id, posicion->x,
			posicion->y)) {
		char* mensaje_error =
				string_from_format(
						"Se quiere mover desde (%d,%d) a (%d,%d): el punto destino esta fuera del mapa o el personaje_id %c no esta cargado",
						personaje->posicion->x, personaje->posicion->y,
						posicion->x, posicion->y, personaje->id);

		mensaje_create_and_send(M_ERROR, string_duplicate(mensaje_error),
				strlen(mensaje_error), client);

		free(mensaje_error);
		return;
	}

	mapa_dibujar(self->mapa);
	posicion_destroy(personaje->posicion);
	personaje->posicion = posicion;

	mensaje_create_and_send(M_SOLICITUD_MOVIMIENTO_OK_RESPONSE, NULL, 0,
			client);

}

void cargar_recursos_config(t_config* config, nivel_t_nivel* new) {

	int index = 1;
	char* key = string_from_format("Caja%d", index);

	while (config_has_property(config, key)) {
		t_recurso* recurso = nivel_create_recurso(
				config_get_string_value(config, key));
		list_add(new->recursos, recurso);
		nivel_create_grafica_recurso(new, recurso);
		free(key);
		key = string_from_format("Caja%d", ++index);
	}

	free(key);
}

t_recurso* nivel_create_recurso(char* config_string) {
//config_string example: Caja1=Flores,F,3,23,5
	char** values = string_split(config_string, ",");

	t_recurso* new_recurso = malloc(sizeof(t_recurso));
	new_recurso->nombre = string_duplicate(values[0]);
	new_recurso->simbolo = values[1][0]; // get first char of string.
	new_recurso->cantidad = atoi(values[2]);

	int x = atoi(values[3]);
	int y = atoi(values[4]);
	new_recurso->posicion = posicion_create(x, y);

	free(values[0]);
	free(values[1]);
	free(values[2]);
	free(values[3]);
	free(values[4]);
	free(values);

	return new_recurso;
}

void nivel_create_grafica_recurso(nivel_t_nivel* self, t_recurso* recurso) {
	mapa_create_caja_recurso(self->mapa, recurso->simbolo, recurso->posicion->x,
			recurso->posicion->y, recurso->cantidad);
}

void verificar_personaje_desconectado(nivel_t_nivel* self,
		t_socket_client* client) {

	bool es_el_personaje(nivel_t_personaje* elem) {
		return sockets_equalsClients(client, elem->socket);
	}

	nivel_t_personaje* personaje_desconectado = list_find(self->personajes,
			(void*) es_el_personaje);

//TODO LIBERAR RECURSOS
	if (personaje_desconectado != NULL ) {
		nivel_loguear(log_warning, self, "El personaje %c se ha desconectado",
				personaje_desconectado->id);
		mapa_borrar_item(self->mapa, personaje_desconectado->id);
		my_list_remove_and_destroy_by_condition(self->personajes,
				(void*) es_el_personaje, (void*) nivel_destroy_personaje);
	}

}

void nivel_create_verificador_deadlock(nivel_t_nivel* self) {
	pthread_t thread_verificador_deadlock;
	pthread_create(&thread_verificador_deadlock, NULL, verificador_deadlock,
			(void*) self);
//	return thread_verificador_deadlock;?
}
