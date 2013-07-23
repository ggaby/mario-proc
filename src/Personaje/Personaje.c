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
	} else {
		log_debug(self->logger, "Personaje creado");
	}

	void handle_signal(int signal) {
		log_info(self->logger, "Señal %d atrapada", signal);
		switch (signal) {
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

	if (!personaje_conectar_a_orquestador(self)) {
		personaje_destroy(self);
		return EXIT_FAILURE;
	}

	if (!personaje_get_info_nivel(self)) {
		personaje_destroy(self);
		return EXIT_FAILURE;
	}

	log_debug(self->logger,
			"Nivel recibido: NIVEL-IP:%s, NIVEL-PUERTO:%d, PLAN-IP:%s, PLAN-PUERTO:%d",
			self->nivel_info->nivel->ip, self->nivel_info->nivel->puerto,
			self->nivel_info->planificador->ip,
			self->nivel_info->planificador->puerto);

	//Hacer esto es una mierda, pero la otra forma es peor ;)
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

	log_info(self->logger, "Conexión terminada");
	personaje_destroy(self);

	return EXIT_SUCCESS;
}

bool personaje_get_info_nivel(t_personaje* self) {

	log_info(self->logger, "Solicitando datos de nivel");
	//TODO: Obvio que acá no siempre en el nivel[0]
	mensaje_create_and_send(M_GET_INFO_NIVEL_REQUEST,
			strdup(self->plan_de_niveles[0]),
			strlen(self->plan_de_niveles[0]) + 1, self->socket_orquestador);

	t_mensaje* response = mensaje_recibir(self->socket_orquestador);

	if (response == NULL ) {
		log_error(self->logger, "Error al recibir la información del nivel");
		return NULL ;
	}

	if (response->type == M_ERROR) {
		log_error(self->logger, (char*) response->payload);
		mensaje_destroy(response);
		return false;
	}

	if (response->type != M_GET_INFO_NIVEL_RESPONSE) {
		log_error(self->logger, "Error desconocido en la respuesta");
		mensaje_destroy(response);
		return false;
	}

	t_get_info_nivel_response* response_data =
			get_info_nivel_response_deserialize((char*) response->payload);

	self->nivel_info = personaje_nivel_create(response_data->nivel,
			response_data->planificador);

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

	new->nivel_info = NULL;
	new->socket_nivel = NULL;
	new->socket_orquestador = NULL;
	new->socket_planificador = NULL;

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
		sockets_destroyClient(self->socket_orquestador);
	}
	if (self->nivel_info != NULL ) {
		personaje_nivel_destroy(self->nivel_info);
	}
	if (self->socket_nivel != NULL ) {
		sockets_destroyClient(self->socket_nivel);
	}
	if (self->socket_planificador != NULL ) {
		sockets_destroyClient(self->socket_planificador);
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

t_personaje_nivel* personaje_nivel_create(t_connection_info* nivel,
		t_connection_info* planificador) {
	t_personaje_nivel* new = malloc(sizeof(t_personaje_nivel));
	new->nivel = nivel;
	new->planificador = planificador;
	return new;
}

void personaje_nivel_destroy(t_personaje_nivel* self) {
	connection_destroy(self->nivel);
	connection_destroy(self->planificador);
	free(self);
}

bool verificar_argumentos(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Error en la cantidad de argumentos.\n");
		return false;
	}
	return true;
}

bool personaje_conectar_a_nivel(t_personaje* self) {
	self->socket_nivel = sockets_conectar_a_servidor(NULL, self->puerto,
			self->nivel_info->nivel->ip, self->nivel_info->nivel->puerto,
			self->logger, M_HANDSHAKE_PERSONAJE, PERSONAJE_HANDSHAKE,
			HANDSHAKE_SUCCESS, "Nivel");

	return (self->socket_nivel != NULL );
}

bool personaje_conectar_a_planificador(t_personaje* self) {
	self->socket_planificador = sockets_conectar_a_servidor(NULL, self->puerto,
			self->nivel_info->planificador->ip,
			self->nivel_info->planificador->puerto, self->logger,
			M_HANDSHAKE_PERSONAJE, PERSONAJE_HANDSHAKE, HANDSHAKE_SUCCESS,
			"Planificador");

	return (self->socket_planificador != NULL );
}
