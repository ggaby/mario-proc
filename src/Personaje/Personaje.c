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

void personaje_perder_vida(int n);
bool verificar_argumentos(int argc, char* argv[]);

t_personaje* self;

int main(int argc, char* argv[]) {

	if (!verificar_argumentos(argc, argv)) {
		return EXIT_FAILURE;
	}

	self = personaje_create(argv[1]);

	if (self == NULL ) {
		return EXIT_FAILURE;
	} else {
		log_debug(self->logger, "Personaje creado");
	}

	signal(SIGUSR1, &personaje_perder_vida);

	t_socket_client* socket_orquestador = personaje_conectar_a_orquestador();

	if (socket_orquestador == NULL) {
		personaje_destroy(self);
		return EXIT_FAILURE;
	}

	t_connection_info* nivel = personaje_get_info_nivel(socket_orquestador);

	if (nivel != NULL ) {
		log_info(self->logger, "Nivel recibido: IP:%s, PUERTO:%d", nivel->ip, nivel->puerto);
		t_connection_destroy(nivel);
	}
	sockets_destroyClient(socket_orquestador);
	log_info(self->logger, "Conexión terminada");
	personaje_destroy(self);

	return EXIT_SUCCESS;
}

t_connection_info* personaje_get_info_nivel(t_socket_client* orquestador) {

	log_info(self->logger, "Solicitando datos de nivel");
	t_mensaje* request = mensaje_create(M_GET_INFO_NIVEL_REQUEST);
	mensaje_setdata(request, strdup(self->plan_de_niveles[0]),
			strlen(self->plan_de_niveles[0]) + 1);
	mensaje_send(request, orquestador);
	mensaje_destroy(request);

	t_socket_buffer* buffer = sockets_recv(orquestador);

	if (buffer == NULL ) {
		log_error(self->logger, "Error al recibir la información del nivel");
		return NULL ;
	}

	t_mensaje* response = mensaje_deserializer(buffer, 0);
	sockets_bufferDestroy(buffer);

	if (response->type == M_ERROR) {
		log_error(self->logger, (char*) response->payload);
		mensaje_destroy(response);
		return NULL ;
	}

	if (response->type != M_GET_INFO_NIVEL_RESPONSE) {
		log_error(self->logger, "Error desconocido en la respuesta");
		mensaje_destroy(response);
		return NULL ;
	}

	t_connection_info* nivel = t_connection_new(response->payload);

	mensaje_destroy(response);

	return nivel;
}

void personaje_perder_vida(int n) {
	log_info(self->logger, "Señal %d atrapada", n);
	switch (n) {
	case SIGUSR1:
		if (self->vidas > 0) {
			self->vidas--;
		} else {
			//TODO: Qué hacemo?
		}
		log_info(self->logger, "Ahora me quedan %d vidas", self->vidas);
		fflush(stdout);
		break;
	}
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
	new->orquestador = t_connection_new(
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
		log_level = string_duplicate(config_get_string_value(config, "logLevel"));
	}
	new->logger = log_create(log_file, "Personaje", true,
			log_level_from_string(log_level));
	config_destroy(config);
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
	t_connection_destroy(self->orquestador);
	log_destroy(self->logger);
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

t_socket_client* personaje_conectar_a_orquestador() {

	t_socket_client* socket_orquestador = sockets_createClient(NULL,
			self->puerto);

	if (socket_orquestador == NULL ) {
		log_error(self->logger, "Error al crear el socket");
		return NULL ;
	}

	if (sockets_connect(socket_orquestador, self->orquestador->ip,
			self->orquestador->puerto) == 0) {
		log_error(self->logger, "Error al conectar con el orquestador");
		sockets_destroyClient(socket_orquestador);
		return NULL ;
	}

	log_info(self->logger, "Conectando con el orquestador...");
	log_debug(self->logger, "Enviando handshake");

	t_mensaje* mensaje = mensaje_create(M_HANDSHAKE_PERSONAJE);
	mensaje_setdata(mensaje, string_duplicate(PERSONAJE_HANDSHAKE),
			strlen(PERSONAJE_HANDSHAKE) + 1);
	mensaje_send(mensaje, socket_orquestador);
	mensaje_destroy(mensaje);

	t_socket_buffer* buffer = sockets_recv(socket_orquestador);

	if (buffer == NULL ) {
		log_error(self->logger, "Error al recibir la respuesta del handshake");
		sockets_destroyClient(socket_orquestador);
		return NULL ;
	}

	t_mensaje* rta_handshake = mensaje_deserializer(buffer, 0);
	sockets_bufferDestroy(buffer);

	if (rta_handshake->length != (strlen(HANDSHAKE_SUCCESS) + 1)
			|| (!string_equals_ignore_case((char*) rta_handshake->payload,
					HANDSHAKE_SUCCESS))) {
		log_error(self->logger, "Error en la respuesta del handshake");
		mensaje_destroy(rta_handshake);
		sockets_destroyClient(socket_orquestador);
		return NULL ;
	}

	mensaje_destroy(rta_handshake);

	log_info(self->logger, "Conectado con el Orquestador: Origen: 127.0.0.1:%d, Destino: %s:%d",
			self->puerto, self->orquestador->ip, self->orquestador->puerto);

	return socket_orquestador;
}

//TODO verificar que se use
personaje_t_nivel* personaje_nivel_create(t_connection_info* nivel, t_connection_info* planificador) {
	personaje_t_nivel* new = malloc(sizeof(personaje_t_nivel));
	new->data = nivel;
	new->planificador = planificador;
	return new;
}

//TODO verificar que se use
void personaje_nivel_destroy(personaje_t_nivel* self) {
	t_connection_destroy(self->data);
	t_connection_destroy(self->planificador);
	free(self);
}

bool verificar_argumentos(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Error en la cantidad de argumentos.\n");
		return false;
	}
	return true;
}
