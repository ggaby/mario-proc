/*
 ============================================================================
 Name        : Personaje.c
 Author      : reyiyo
 Version     :
 Copyright   : GPLv3
 Description : Main del proceso Personaje
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <commons/string.h>
#include "Personaje.h"

bool verificar_argumentos(int argc, char* argv[]);

int main(int argc, char* argv[]) {

	if (!verificar_argumentos(argc, argv)) {
		return EXIT_FAILURE;
	}

	t_personaje* self = personaje_create(argv[1]);

	if (self == NULL ) {
		return EXIT_FAILURE;
	}

	log_debug(self->logger, "Personaje %s creado", self->nombre);

	void handle_signal(int signal) {
		log_info(self->logger, "Señal %d atrapada", signal);
		switch (signal) { //Tecnicamente aca solo llegaria SIGUSR1 no haria falta el case.
		case SIGUSR1:
			if (self->vidas > 0) {
				self->vidas--;
				log_info(self->logger, "Ahora me quedan %d vidas", self->vidas);
			} else {
				//TODO: Qué hacemo?
			}
			break;
		}
	}
	signal(SIGUSR1, &handle_signal);

	int nivelIndex = 0;
	while (self->plan_de_niveles[nivelIndex] != NULL ) {

		self->nivel_actual = personaje_nivel_create(
				self->plan_de_niveles[nivelIndex]);

		log_info(self->logger, "Personaje %s: comenzando nivel %s",
				self->nombre, self->nivel_actual->nombre);

		if (!personaje_conectar_a_orquestador(self)) {
			personaje_destroy(self);
			return EXIT_FAILURE;
		}

		personaje_avisar_fin_de_nivel(self);

		if (!personaje_get_info_nivel(self)) {
			personaje_destroy(self);
			return EXIT_FAILURE;
		}

		log_debug(self->logger,
				"Nivel recibido: NIVEL-IP:%s, NIVEL-PUERTO:%d, PLAN-IP:%s, PLAN-PUERTO:%d",
				self->nivel_actual->nivel->ip,
				self->nivel_actual->nivel->puerto,
				self->nivel_actual->planificador->ip,
				self->nivel_actual->planificador->puerto);

		//Hacer esto es una mierda, pero la otra forma es peor ;)
		if (self->socket_orquestador->serv_socket != NULL ) {
			sockets_destroy(self->socket_orquestador->serv_socket);
		}
		sockets_destroyClient(self->socket_orquestador);
		self->socket_orquestador = NULL;
		//

		if (!personaje_conectar_a_nivel(self)) {
			personaje_destroy(self);
			return EXIT_FAILURE;
		}

		if (!personaje_conectar_a_planificador(self)) {
			personaje_destroy(self);
			return EXIT_FAILURE;
		}

		if (!personaje_jugar_nivel(self)) {
			personaje_destroy(self);
			return EXIT_FAILURE;
		}

		//TODO Seguir aca el flujo de logica del nivel.

		nivelIndex++;
	} //FIN while por cada nivel.

	if (self->plan_de_niveles[nivelIndex] != NULL ) { //terminé todos los niveles
		//Demian, ya se que está de más, pero me gusta chequearlo al pedo :P
		if (!personaje_conectar_a_orquestador(self)) {
			personaje_destroy(self);
			return EXIT_FAILURE;
		}

		personaje_avisar_fin_de_nivel(self);

		if (self->socket_orquestador->serv_socket != NULL ) {
			sockets_destroy(self->socket_orquestador->serv_socket);
		}
		sockets_destroyClient(self->socket_orquestador);
		self->socket_orquestador = NULL;

		log_info(self->logger, "Terminé todos los niveles :)");
	}

	personaje_destroy(self);

	return EXIT_SUCCESS;
}

bool personaje_get_info_nivel(t_personaje* self) {

	log_info(self->logger, "Personaje %s: Solicitando datos del nivel %s",
			self->nombre, self->nivel_actual->nombre);

	mensaje_create_and_send(M_GET_INFO_NIVEL_REQUEST,
			strdup(self->nivel_actual->nombre),
			strlen(self->nivel_actual->nombre) + 1, self->socket_orquestador);

	t_mensaje* response = mensaje_recibir(self->socket_orquestador);

	if (response == NULL ) {
		log_error(self->logger,
				"Personaje %s: Error al recibir la información del nivel",
				self->nombre);
		return NULL ;
	}

	if (response->type == M_ERROR) {
		log_error(self->logger, "Personaje %s: %s", self->nombre,
				(char*) response->payload);
		mensaje_destroy(response);
		return false;
	}

	if (response->type != M_GET_INFO_NIVEL_RESPONSE) {
		log_error(self->logger,
				"Personaje %s: Error desconocido en la respuesta",
				self->nombre);
		mensaje_destroy(response);
		return false;
	}

	t_get_info_nivel_response* response_data =
			get_info_nivel_response_deserialize((char*) response->payload);

	self->nivel_actual->nivel = response_data->nivel;
	self->nivel_actual->planificador = response_data->planificador;

	free(response_data);
	mensaje_destroy(response);
	return true;
}

t_personaje* personaje_create(char* config_path) {
	t_personaje* new = malloc(sizeof(t_personaje));
	t_config* config = config_create(config_path);

	new->nombre = string_duplicate(config_get_string_value(config, "nombre"));

	char* s = string_duplicate(config_get_string_value(config, "simbolo"));
	new->simbolo = s[0];

	new->plan_de_niveles = config_get_array_value(config, "planDeNiveles");
	new->objetivos = _personaje_load_objetivos(config, new->plan_de_niveles);
	new->vidas = config_get_int_value(config, "vidas");
	new->orquestador_info = connection_create(
			config_get_string_value(config, "orquestador"));

	void morir(char* mensaje) {
		config_destroy(config);
		free(s);
		personaje_destroy(new);
		printf("Error en el archivo de configuración: %s\n", mensaje);
	}

	if (!config_has_property(config, "puerto")) {
		morir("Falta el puerto");
		return NULL ;
	}
	new->puerto = config_get_int_value(config, "puerto");

	char* log_file = "personaje.log";
	char* log_level = "INFO";
	if (config_has_property(config, "logFile")) {
		log_file = string_duplicate(config_get_string_value(config, "logFile"));
	}
	if (config_has_property(config, "logLevel")) {
		log_level = string_duplicate(
				config_get_string_value(config, "logLevel"));
	}
	new->logger = log_create(log_file, "Personaje", true,
			log_level_from_string(log_level));
	config_destroy(config);

	new->socket_orquestador = NULL;
	new->nivel_actual = NULL;
	new->posicion = NULL;
	new->posicion_objetivo = NULL;
	new->nivel_finalizado = false;

	free(s);
	free(log_file);
	free(log_level);
	return new;
}

void personaje_destroy(t_personaje* self) {
	free(self->nombre);
	array_destroy(self->plan_de_niveles);
	dictionary_destroy_and_destroy_elements(self->objetivos,
			(void*) array_destroy);
	connection_destroy(self->orquestador_info);
	log_destroy(self->logger);
	if (self->socket_orquestador != NULL ) {
		if (self->socket_orquestador->serv_socket != NULL ) {
			sockets_destroy(self->socket_orquestador->serv_socket);
		}
		sockets_destroyClient(self->socket_orquestador);
	}
	if (self->nivel_actual != NULL ) {
		personaje_nivel_destroy(self->nivel_actual);
	}
	if (self->posicion != NULL ) {
		posicion_destroy(self->posicion);
	}

	if (self->posicion_objetivo != NULL ) {
		posicion_destroy(self->posicion_objetivo);
	}

	free(self);
}

t_dictionary* _personaje_load_objetivos(t_config* config,
		char** plan_de_niveles) {
	t_dictionary* objetivos = dictionary_create();
	int i = 0;
	while (plan_de_niveles[i] != NULL ) {
		char* nivel = string_duplicate(plan_de_niveles[i]);
		char* key = string_new();
		string_append(&key, "obj[");
		string_append(&key, nivel);
		string_append(&key, "]");
		dictionary_put(objetivos, nivel, config_get_array_value(config, key));
		free(key);
		free(nivel);
		i++;
	}
	return objetivos;
}

bool personaje_conectar_a_orquestador(t_personaje* self) {

	self->socket_orquestador = sockets_conectar_a_servidor(NULL, self->puerto,
			self->orquestador_info->ip, self->orquestador_info->puerto,
			self->logger, M_HANDSHAKE_PERSONAJE, PERSONAJE_HANDSHAKE,
			HANDSHAKE_SUCCESS, "Orquestador");

	return (self->socket_orquestador != NULL );
}

t_personaje_nivel* personaje_nivel_create(char* nombre_nivel) {
	t_personaje_nivel* new = malloc(sizeof(t_personaje_nivel));
	new->nombre = string_duplicate(nombre_nivel);
	new->nivel = NULL;
	new->planificador = NULL;
	new->socket_nivel = NULL;
	new->socket_planificador = NULL;
	return new;
}

void personaje_nivel_destroy(t_personaje_nivel* nivel) {

	free(nivel->nombre);

	if (nivel->nivel != NULL ) {
		connection_destroy(nivel->nivel);
	}

	if (nivel->planificador != NULL ) {
		connection_destroy(nivel->planificador);
	}

	if (nivel->socket_nivel != NULL ) {
		if (nivel->socket_nivel->serv_socket != NULL ) {
			sockets_destroy(nivel->socket_nivel->serv_socket);
		}
		sockets_destroyClient(nivel->socket_nivel);
	}

	if (nivel->socket_planificador != NULL ) {
		if (nivel->socket_planificador->serv_socket != NULL ) {
			sockets_destroy(nivel->socket_planificador->serv_socket);
		}
		sockets_destroyClient(nivel->socket_planificador);
	}

	free(nivel);
}

bool verificar_argumentos(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Error en la cantidad de argumentos.\n");
		return false;
	}
	return true;
}

bool personaje_conectar_a_nivel(t_personaje* self) {
	self->nivel_actual->socket_nivel = sockets_conectar_a_servidor(NULL,
			self->puerto, self->nivel_actual->nivel->ip,
			self->nivel_actual->nivel->puerto, self->logger,
			M_HANDSHAKE_PERSONAJE, PERSONAJE_HANDSHAKE, HANDSHAKE_SUCCESS,
			"Nivel");

	if (self->nivel_actual->socket_nivel == NULL ) {
		return false;
	}

	t_mensaje* mensaje = mensaje_recibir(self->nivel_actual->socket_nivel);

	if (mensaje == NULL ) {
		log_error(self->logger, "Personaje %s: El nivel %s se ha desconectado.",
				self->nombre, self->nivel_actual->nombre);
		return false;
	}

	if (mensaje->type != M_GET_SYMBOL_PERSONAJE_REQUEST) {
		mensaje_destroy(mensaje);
		return false;
	}

	mensaje_destroy(mensaje);

	char* simbolo = string_from_format("%c", self->simbolo);

	mensaje_create_and_send(M_GET_SYMBOL_PERSONAJE_RESPONSE,
			string_duplicate(simbolo), strlen(simbolo) + 1,
			self->nivel_actual->socket_nivel);
	free(simbolo);

	if (self->posicion != NULL ) {
		posicion_destroy(self->posicion);
	}
	self->posicion = posicion_create(0, 0);

	return true;
}

bool personaje_conectar_a_planificador(t_personaje* self) {
	self->nivel_actual->socket_planificador = sockets_conectar_a_servidor(NULL,
			self->puerto, self->nivel_actual->planificador->ip,
			self->nivel_actual->planificador->puerto, self->logger,
			M_HANDSHAKE_PERSONAJE, PERSONAJE_HANDSHAKE, HANDSHAKE_SUCCESS,
			"Planificador");

	if (self->nivel_actual->socket_planificador == NULL ) {
		return false;
	}

	t_mensaje* mensaje = mensaje_recibir(self->nivel_actual->socket_planificador);

	if (mensaje == NULL ) {
		log_error(self->logger, "Personaje %s: El planificador del nivel %s se ha desconectado.",
				self->nombre, self->nivel_actual->nombre);
		return false;
	}

	if (mensaje->type != M_GET_SYMBOL_PERSONAJE_REQUEST) {
		mensaje_destroy(mensaje);
		return false;
	}

	mensaje_destroy(mensaje);

	char* simbolo = string_from_format("%c", self->simbolo);

	mensaje_create_and_send(M_GET_SYMBOL_PERSONAJE_RESPONSE,
			string_duplicate(simbolo), strlen(simbolo) + 1,
			self->nivel_actual->socket_planificador);
	free(simbolo);

	return true;
}

bool personaje_jugar_nivel(t_personaje* self) {
	log_info(self->logger, "Personaje %s: comenzando nivel %s", self->nombre,
			self->nivel_actual->nombre);

	char** objetivos = dictionary_get(self->objetivos,
			self->nivel_actual->nombre);

	int objetivos_index = 0;
	char* objetivo = NULL;

	while (objetivos[objetivos_index] != NULL ) {
		objetivo = string_duplicate(objetivos[objetivos_index++]); //incremento aca para contabilizar solo cuando cambio de objetivo

		log_info(self->logger, "Personaje %s: nuevo objetivo %s", self->nombre,
				objetivo);

		if (self->posicion_objetivo == NULL ) {
			self->posicion_objetivo = pedir_posicion_objetivo(self, objetivo);
		}

		if (self->posicion_objetivo == NULL ) {
			free(objetivo);
			return false;
		}

		while (!posicion_equals(self->posicion, self->posicion_objetivo)) {

			if (!realizar_movimiento(self)) {
				free(objetivo);
				return false;
			}

			if (!finalizar_turno(self, objetivo)) {
				free(objetivo);
				return false;
			}

		}
		posicion_destroy(self->posicion_objetivo);
		self->posicion_objetivo = NULL;
		free(objetivo);
	}

	finalizar_nivel(self);

	return true;
}

t_posicion* pedir_posicion_objetivo(t_personaje* self, char* objetivo) {

	mensaje_create_and_send(M_GET_POSICION_RECURSO_REQUEST,
			string_duplicate(objetivo), strlen(objetivo) + 1,
			self->nivel_actual->socket_nivel);

	t_mensaje* mensaje = mensaje_recibir(self->nivel_actual->socket_nivel);

	if (mensaje == NULL ) {
		log_error(self->logger, "Personaje %s: El nivel %s se ha desconectado.",
				self->nombre, self->nivel_actual->nombre);
		return NULL ;
	}

	t_posicion* posicion_objetivo = posicion_duplicate(mensaje->payload);
	mensaje_destroy(mensaje);

	log_info(self->logger, "Personaje %s: %s esta en (%d,%d)", self->nombre,
			objetivo, posicion_objetivo->x, posicion_objetivo->y);
	return posicion_objetivo;
}

bool realizar_movimiento(t_personaje* self) {
	t_mensaje* notificacion_movimiento = mensaje_recibir(
			self->nivel_actual->socket_planificador);

	if (notificacion_movimiento == NULL ) {
		log_error(self->logger,
				"Personaje %s: El planificador se ha desconectado.",
				self->nombre);
		return false;
	}

	int mensaje_type = notificacion_movimiento->type;
	mensaje_destroy(notificacion_movimiento);

	if (mensaje_type == M_NOTIFICACION_MOVIMIENTO) {
		return mover_en_nivel(self);
	}
	return false;
}

bool mover_en_nivel(t_personaje* self) {
	t_posicion* proxima_posicion = posicion_get_proxima_hacia(self->posicion,
			self->posicion_objetivo);

	mensaje_create_and_send(M_SOLICITUD_MOVIMIENTO_REQUEST,
			posicion_duplicate(proxima_posicion), sizeof(t_posicion),
			self->nivel_actual->socket_nivel);

	t_mensaje* solicitud_mov_response = mensaje_recibir(
			self->nivel_actual->socket_nivel);

	if (solicitud_mov_response == NULL ) {
		log_error(self->logger, "Personaje %s: El nivel %s se ha desconectado.",
				self->nombre, self->nivel_actual->nombre);
		posicion_destroy(proxima_posicion);
		return false;
	}

	if (solicitud_mov_response->type == M_ERROR) {
		log_error(self->logger, "Movimiento rechazado: %s",
				(char*) solicitud_mov_response->payload);
		mensaje_destroy(solicitud_mov_response);
		posicion_destroy(proxima_posicion);
		return false;
	}

	if (solicitud_mov_response->type == M_SOLICITUD_MOVIMIENTO_OK_RESPONSE) {
		posicion_destroy(self->posicion);
		self->posicion = proxima_posicion;
		mensaje_destroy(solicitud_mov_response);
		return true;
	}
	mensaje_destroy(solicitud_mov_response);
	posicion_destroy(proxima_posicion);
	return false;
}

//TODO: Revisar los returns y ver si se puede mejorar el if... o no :P
bool finalizar_turno(t_personaje* self, char* objetivo) {

	if (posicion_equals(self->posicion, self->posicion_objetivo)) { //Llegué al recurso
		log_info(self->logger, "Personaje %s: objetivo %s alcanzado",
				self->nombre, objetivo);
		t_mensaje* result = solicitar_recurso(self);

		if (result == NULL ) {
			return false;
		}

		if (result->type == M_SOLICITUD_RECURSO_RESPONSE_OK) {
			log_info(self->logger, "Personaje %s: objetivo %s asignado",
					self->nombre, objetivo);
			mensaje_create_and_send(M_TURNO_FINALIZADO_OK, NULL, 0,
					self->nivel_actual->socket_planificador);
			mensaje_destroy(result);
			return true;
		}

		if (result->type == M_SOLICITUD_RECURSO_RESPONSE_BLOCKED) {
			log_info(self->logger, "Personaje %s: objetivo %s NO asignado",
					self->nombre, objetivo);
			mensaje_create_and_send(M_TURNO_FINALIZADO_BLOCKED,
					string_duplicate(objetivo), strlen(objetivo) + 1,
					self->nivel_actual->socket_planificador);
			mensaje_destroy(result);
			return true;
		}
		mensaje_destroy(result);
		return false;
	} else {
		mensaje_create_and_send(M_TURNO_FINALIZADO_OK, NULL, 0,
				self->nivel_actual->socket_planificador);
		return true;
	}
	return false;
}

t_mensaje* solicitar_recurso(t_personaje* self) {
	mensaje_create_and_send(M_SOLICITUD_RECURSO_REQUEST,
			posicion_duplicate(self->posicion), sizeof(t_posicion),
			self->nivel_actual->socket_nivel);

	t_mensaje* recurso_response = mensaje_recibir(
			self->nivel_actual->socket_nivel);

	if (recurso_response == NULL ) {
		return NULL ;
	}
	return recurso_response;

}

void finalizar_nivel(t_personaje* self) {
	log_info(self->logger, "Personaje %s: finalice nivel %s", self->nombre,
			self->nivel_actual->nombre);
	self->nivel_finalizado = true;

	mensaje_create_and_send(M_FIN_DE_NIVEL, NULL, 0,
			self->nivel_actual->socket_nivel);

	personaje_nivel_destroy(self->nivel_actual);

	if (self->posicion != NULL ) {
		posicion_destroy(self->posicion);
		self->posicion = NULL;
	}

	if (self->posicion_objetivo != NULL ) {
		posicion_destroy(self->posicion_objetivo);
		self->posicion_objetivo = NULL;
	}
}

void personaje_avisar_fin_de_nivel(t_personaje* self) {
	if (self->nivel_finalizado) {
		log_debug(self->logger,
				"Personaje %s: Avisando al orquestador que terminé el nivel", self->nombre);
		mensaje_create_and_send(M_FIN_DE_NIVEL, NULL, 0,
				self->socket_orquestador);
	}
}
