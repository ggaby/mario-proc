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
void nivel_mover_personaje(nivel_t_nivel* self, t_posicion* posicion,
		t_socket_client* client);

void nivel_bloquear_personaje(nivel_t_personaje* personaje, t_recurso* recurso);
void nivel_desbloquear_personaje(nivel_t_personaje* personaje);

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

		char* mi_nombre = string_from_format("Nivel %s", self->nombre);
		responder_handshake(client, self->logger, &self->logger_mutex,
				mi_nombre);

		free(mi_nombre);

		char* id_personaje = mensaje_get_simbolo_personaje(client, self->logger,
				&self->logger_mutex);

		if (!id_personaje) {
			return NULL ;
		}

		nivel_loguear(log_debug, self, "ID de personaje recibido: %s",
				id_personaje);

		list_add(self->personajes,
				nivel_create_personaje(self, id_personaje[0], client));
		free(id_personaje);

		return client;
	}

	int recvClosure(t_socket_client* client) {

		t_mensaje* mensaje = mensaje_recibir(client);

		if (mensaje == NULL ) {
			nivel_loguear(log_warning, self, "Mensaje recibido NULL.");
			verificar_personaje_desconectado(self, client, false);
			return false;
		}

		if (!nivel_process_request(self, mensaje, client)) {
			mensaje_destroy(mensaje);
			return false;
		}

		mensaje_destroy(mensaje);
		return true;
	}

	void onSelectClosure() {
		mapa_dibujar(self->mapa);
	}

	nivel_create_verificador_deadlock(self);

	sockets_create_little_server(NULL, self->puerto, self->logger,
			&self->logger_mutex, self->nombre, self->servers, self->clients,
			&acceptClosure, &recvClosure, &onSelectClosure);

	nivel_loguear(log_info, self, "cerrado correctamente");
	nivel_destroy(self);

	return EXIT_SUCCESS;
}

nivel_t_nivel* nivel_create(char* config_path) {
	nivel_t_nivel* new = malloc(sizeof(nivel_t_nivel));
	t_config* config = config_create(config_path);
	new->nombre = string_duplicate(config_get_string_value(config, "Nombre"));
	new->tiempoChequeoDeadlock = config_get_double_value(config,
			"TiempoChequeoDeadlock") * 1000000;
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
		free(log_file);
		log_file = string_duplicate(config_get_string_value(config, "logFile"));
	}
	if (config_has_property(config, "logLevel")) {
		free(log_level);
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

	mapa_destroy(self->mapa);

	list_destroy_and_destroy_elements(self->personajes,
			(void*) nivel_destroy_personaje);
	list_destroy_and_destroy_elements(self->recursos, (void*) recurso_destroy);

	list_destroy_and_destroy_elements(self->clients,
			(void*) sockets_destroyClient);
	list_destroy_and_destroy_elements(self->servers,
			(void*) sockets_destroyServer);

	free(self);
}

void nivel_destroy_personaje(nivel_t_personaje* personaje) {
	if (personaje->posicion != NULL ) {
		posicion_destroy(personaje->posicion);
	}
	nivel_desbloquear_personaje(personaje);
	list_destroy_and_destroy_elements(personaje->recursos_asignados,
			(void*) recurso_destroy);
	free(personaje);
}

bool nivel_process_request(nivel_t_nivel* self, t_mensaje* request,
		t_socket_client* client) {
	switch (request->type) {
	case M_GET_NOMBRE_NIVEL_REQUEST:
		nivel_get_nombre(self, client);
		break;
	case M_GET_POSICION_RECURSO_REQUEST:
		nivel_get_posicion_recurso(self, request->payload, client);
		break;
	case M_SOLICITUD_MOVIMIENTO_REQUEST:
		nivel_mover_personaje(self,
				posicion_duplicate((t_posicion*) request->payload), client);
		break;
	case M_SOLICITUD_RECURSO_REQUEST:
		nivel_asignar_recurso(self, (t_posicion*) request->payload, client);
		break;
	case M_FIN_DE_NIVEL:
		verificar_personaje_desconectado(self, client, true);
		break;
	case M_RECURSOS_ASIGNADOS:
		nivel_asignar_recursos_liberados(self,
				string_duplicate((char*) request->payload), client);
		break;
	case M_MUERTE_PERSONAJE:
		verificar_personaje_desconectado(self, client, false);
			break;
	default: {
		char* error_msg = string_from_format(
				"Tipo del mensaje recibido no valido tipo: %d", request->type);
		nivel_loguear(log_warning, self, error_msg);
		mensaje_create_and_send(M_ERROR, string_duplicate(error_msg),
				strlen(error_msg) + 1, client);
		free(error_msg);
	}
		return false;
	}
	return true;
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
	char* msg = string_from_vformat(template, arguments);
	log_fn(self->logger, msg);
	free(template);
	free(msg);
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
				"El recurso %s no se encuentra en el nivel.", id_recurso);

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
	personaje->simbolo_recurso_esperado = NULL;

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

	nivel_desbloquear_personaje(personaje);

	int distancia = posicion_get_distancia(personaje->posicion, posicion);

	nivel_loguear(log_debug, self, "Personaje: (%d,%d), pos: (%d,%d)",
			personaje->posicion->x, personaje->posicion->y, posicion->x,
			posicion->y);

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
		t_recurso* recurso = recurso_from_config_string(
				config_get_string_value(config, key));
		list_add(new->recursos, recurso);
		nivel_create_grafica_recurso(new, recurso);
		free(key);
		key = string_from_format("Caja%d", ++index);
	}

	free(key);
}

void nivel_create_grafica_recurso(nivel_t_nivel* self, t_recurso* recurso) {
	mapa_create_caja_recurso(self->mapa, recurso->simbolo, recurso->posicion->x,
			recurso->posicion->y, recurso->cantidad);
}

void verificar_personaje_desconectado(nivel_t_nivel* self,
		t_socket_client* client, bool fin_de_nivel) {

	bool es_el_personaje(nivel_t_personaje* elem) {
		return sockets_equalsClients(client, elem->socket);
	}

	nivel_t_personaje* personaje_desconectado = list_find(self->personajes,
			(void*) es_el_personaje);

	if (personaje_desconectado != NULL ) {
		if (fin_de_nivel) {
			nivel_loguear(log_warning, self,
					"El personaje %c se ha terminado el nivel",
					personaje_desconectado->id);
		} else {
			nivel_loguear(log_warning, self,
					"El personaje %c se ha desconectado",
					personaje_desconectado->id);
		}
		nivel_liberar_recursos(self,
				personaje_desconectado->recursos_asignados);
		mapa_borrar_item(self->mapa, personaje_desconectado->id);
		if (!fin_de_nivel) {
			my_list_remove_and_destroy_by_condition(self->personajes,
					(void*) es_el_personaje, (void*) nivel_destroy_personaje);
		}
	}

}

void nivel_create_verificador_deadlock(nivel_t_nivel* self) {
	pthread_t thread_verificador_deadlock;
	pthread_create(&thread_verificador_deadlock, NULL, verificador_deadlock,
			(void*) self);
//	return thread_verificador_deadlock;?
}

void nivel_asignar_recurso(nivel_t_nivel* self, t_posicion* posicion,
		t_socket_client* client) {

	bool es_el_recurso(t_recurso* elem) {
		return posicion_equals(elem->posicion, posicion);
	}

	bool es_el_personaje(nivel_t_personaje* elem) {
		return sockets_equalsClients(elem->socket, client);
	}

	t_recurso* el_recurso = list_find(self->recursos, (void*) es_el_recurso);

	if (el_recurso == NULL ) {
		char* error_msg = string_from_format(
				"Recurso solicitado inválido (%d,%d)", posicion->x,
				posicion->y);
		nivel_loguear(log_warning, self, error_msg);
		mensaje_create_and_send(M_ERROR, string_duplicate(error_msg),
				strlen(error_msg) + 1, client);
		free(error_msg);
		return;
	}

	nivel_t_personaje* el_personaje = list_find(self->personajes,
			(void*) es_el_personaje);

	if (el_personaje == NULL ) {
		nivel_loguear(log_warning, self,
				"Parece que el personaje se desconectó solicitando un recurso");
		verificar_personaje_desconectado(self, client, false);
		return;
	}

	if (!posicion_equals(el_personaje->posicion, el_recurso->posicion)) {
		char* msg = "No estás ahí, no quieras hacer trampa";
		mensaje_create_and_send(M_ERROR, msg, strlen(msg) + 1, client);
		free(msg);
		return;
	}

	if (el_recurso->cantidad > 0) {
		asignar_recurso_a_personaje(self, el_personaje, el_recurso);
		el_recurso->cantidad--;
		mapa_update_recurso(self->mapa, el_recurso->simbolo,
				el_recurso->cantidad);
		mensaje_create_and_send(M_SOLICITUD_RECURSO_RESPONSE_OK, NULL, 0,
				client);
	} else {
		nivel_loguear(log_info, self, "Recursos %s insuficientes, bloqueando",
				el_recurso->nombre);

		nivel_bloquear_personaje(el_personaje, el_recurso);

		mensaje_create_and_send(M_SOLICITUD_RECURSO_RESPONSE_BLOCKED,
				string_duplicate(el_recurso->nombre),
				strlen(el_recurso->nombre) + 1, client);
	}
}

void asignar_recurso_a_personaje(nivel_t_nivel* self,
		nivel_t_personaje* personaje, t_recurso* recurso) {

	bool es_el_recurso(t_recurso* elem) {
		return recurso_equals(recurso, elem);
	}

	t_recurso* recurso_asignado = list_find(personaje->recursos_asignados,
			(void*) es_el_recurso);

	if (recurso_asignado == NULL ) {
		recurso_asignado = recurso_clone(recurso);
		recurso_asignado->cantidad = 1;
		list_add(personaje->recursos_asignados, recurso_asignado);
	} else {
		recurso_asignado->cantidad++;
	}
}

void nivel_liberar_recursos(nivel_t_nivel* self, t_list* recursos) {
	t_list* recursos_liberados = list_create();

	void liberar_recurso(t_recurso* recurso) {
		bool es_el_recurso(t_recurso* elem) {
			return recurso_equals(recurso, elem);
		}

		t_recurso* mi_recurso = list_find(self->recursos,
				(void*) es_el_recurso);
		mi_recurso->cantidad += recurso->cantidad;
		nivel_loguear(log_info, self,
				"Se liberaron %d instancias del recurso %s", recurso->cantidad,
				recurso->nombre);
		mapa_update_recurso(self->mapa, mi_recurso->simbolo,
				mi_recurso->cantidad);
		list_add(recursos_liberados, recurso_clone(recurso));
		my_list_remove_and_destroy_by_condition(recursos, (void*) es_el_recurso,
				(void*) recurso_destroy);
	}

	list_iterate(recursos, (void *) liberar_recurso);

	avisar_al_orquestador_que_se_liberaron_recursos(self, recursos_liberados);

	list_destroy_and_destroy_elements(recursos_liberados,
			(void*) recurso_destroy);
}

void nivel_bloquear_personaje(nivel_t_personaje* personaje, t_recurso* recurso) {
	nivel_desbloquear_personaje(personaje);
	personaje->simbolo_recurso_esperado = string_from_format("%c",
			recurso->simbolo);
}

void nivel_desbloquear_personaje(nivel_t_personaje* personaje) {
	if (personaje->simbolo_recurso_esperado != NULL ) {
		free(personaje->simbolo_recurso_esperado);
		personaje->simbolo_recurso_esperado = NULL;
	}
}

void avisar_al_orquestador_que_se_liberaron_recursos(nivel_t_nivel* self,
		t_list* recursos) {
	char* recursos_a_liberar = string_new();

	void agregar_recurso(t_recurso* recurso) {
		char* rec_str = string_from_format("%c%d,", recurso->simbolo,
				recurso->cantidad);
		string_append(&recursos_a_liberar, string_duplicate(rec_str));
		free(rec_str);
	}

	list_iterate(recursos, (void*) agregar_recurso);

	if (strlen(recursos_a_liberar) > 0) {
		mensaje_create_and_send(M_RECURSOS_LIBERADOS,
				string_duplicate(recursos_a_liberar),
				strlen(recursos_a_liberar) + 1, self->socket_orquestador);
	}
	free(recursos_a_liberar);
}

void nivel_asignar_recursos_liberados(nivel_t_nivel* self,
		char* recursos_asignados_str, t_socket_client* client) {

	nivel_loguear(log_debug, self, "Recursos para asignar: %s",
			recursos_asignados_str);

	if (strlen(recursos_asignados_str) < 2) {
		return;
	}
	char** asignaciones = parsear_recursos_asignados(recursos_asignados_str);

	void buscar_y_asignar(char* asignacion) {

		bool es_el_personaje(nivel_t_personaje* elem) {
			return elem->id == asignacion[0];
		}

		bool es_el_recurso(t_recurso* elem) {
			return asignacion[1] == elem->simbolo;
		}

		nivel_t_personaje* el_personaje = list_find(self->personajes,
				(void*) es_el_personaje);
		t_recurso* el_recurso = list_find(self->recursos,
				(void*) es_el_recurso);

		if (el_personaje != NULL && el_recurso != NULL ) {
			asignar_recurso_a_personaje(self, el_personaje, el_recurso);
			el_recurso->cantidad--;
			mapa_update_recurso(self->mapa, el_recurso->simbolo,
					el_recurso->cantidad);

			if ((el_personaje->simbolo_recurso_esperado == NULL ) ||
			(el_personaje->simbolo_recurso_esperado[0] != el_recurso->simbolo)){

			nivel_loguear(log_debug, self,
					"Algo raro pasó, porque el personaje no estaba bloqueado");

		}
			nivel_desbloquear_personaje(el_personaje);
			nivel_loguear(log_debug, self,
					"Se asigno 1 instancias del recurso %s al persoanje %c",
					el_recurso->nombre, el_personaje->id);
		}

	}

	string_iterate_lines(asignaciones, (void*) buscar_y_asignar);
	array_destroy(asignaciones);
}

char** parsear_recursos_asignados(char* recursos_str) {
	char* recursos_str2 = string_substring(recursos_str, 0,
			strlen(recursos_str) - 1);
	return string_split(recursos_str2, ",");
}
