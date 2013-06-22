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
	signal(SIGUSR1, &personaje_perder_vida);
	printf("Personaje creado\n");

	t_socket_client* socket_orquestador = sockets_createClient(NULL,
			self->puerto);

	if (socket_orquestador == NULL ) {
		personaje_destroy(self);
		return EXIT_FAILURE;
	}

	if (sockets_connect(socket_orquestador, self->orquestador->ip,
			self->orquestador->puerto) == 0) {
		sockets_destroyClient(socket_orquestador);
		personaje_destroy(self);
	}

	printf("Conectado con la plataforma\n");
	printf("Enviando handshake\n");

	t_mensaje* mensaje = mensaje_create(M_HANDSHAKE);
	mensaje_setdata(mensaje, strdup("Aquí un personaje"),
			strlen("Aquí un personaje") + 1);
	printf("DATA: %s\n", (char*) mensaje_getdata(mensaje));
	mensaje_send(mensaje, socket_orquestador);
	mensaje_destroy(mensaje);

	printf("Recibiendo resultado del handshake\n");
	t_socket_buffer* buffer = sockets_recv(socket_orquestador);

	if (buffer == NULL ) {
		printf("Error en el resultado\n");
		sockets_destroyClient(socket_orquestador);
		personaje_destroy(self);
		return EXIT_FAILURE;
	}

	t_mensaje* rta_handshake = mensaje_deserializer(buffer, 0);
	sockets_bufferDestroy(buffer);

	if (rta_handshake->length != (strlen("OK") + 1)
			|| (!string_equals_ignore_case((char*) rta_handshake->payload, "OK"))) {
		printf("Error en el LENGTH del resultado\n");
		mensaje_destroy(rta_handshake);
		sockets_destroyClient(socket_orquestador);
		personaje_destroy(self);
		return EXIT_FAILURE;
	}

	printf("TYPE: %d\n", rta_handshake->type);
	printf("LENGHT: %d\n", rta_handshake->length);
	printf("DATA: %s\n", (char*) rta_handshake->payload);
	mensaje_destroy(rta_handshake);

	t_connection_info* nivel = personaje_get_info_nivel(socket_orquestador);

	if (nivel != NULL) {
		t_connection_destroy(nivel);
	}
	personaje_destroy(self);
	sockets_destroyClient(socket_orquestador);

	printf("Conexión terminada\n");
	return EXIT_SUCCESS;
}

t_connection_info* personaje_get_info_nivel(t_socket_client* orquestador) {
	t_mensaje* request = mensaje_create(M_GET_INFO_NIVEL_REQUEST);
	mensaje_setdata(request, strdup(self->plan_de_niveles[0]),
			strlen(self->plan_de_niveles[0]) + 1);
	printf("DATA: %s\n", (char*) mensaje_getdata(request));
	mensaje_send(request, orquestador);
	mensaje_destroy(request);

	printf("Recibiendo Info nivel\n");
	t_socket_buffer* buffer = sockets_recv(orquestador);

	if (buffer == NULL ) {
		printf("Error en el resultado\n");
		free(buffer);
		return NULL ;
	}

	t_mensaje* response = mensaje_deserializer(buffer, 0);
	sockets_bufferDestroy(buffer);

	if (response->type == M_ERROR) {
		printf("Error: %s\n", (char*) response->payload);
		mensaje_destroy(response);
		return NULL;
	}

	if (response->type != M_GET_INFO_NIVEL_RESPONSE) {
		printf("Error desconocido\n");
		mensaje_destroy(response);
		return NULL;
	}

	t_connection_info* nivel = t_connection_new(response->payload);

	printf("TYPE: %d\n", response->type);
	printf("LENGHT: %d\n", response->length);
	printf("DATA: IP: %s, PUERTO: %d\n", nivel->ip, nivel->puerto);
	mensaje_destroy(response);

	return nivel;
}

void personaje_perder_vida(int n) {
	printf("Perdiendo vida!\n");
	switch (n) {
	case SIGUSR1:
		if (self->vidas > 0) {
			self->vidas--;
		} else {
			//TODO: Qué hacemo?
		}
		printf("Ahora me quedan %d vidas\n", self->vidas);
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
	new->puerto = config_get_int_value(config, "puerto");
	config_destroy(config);
	free(s);
	return new;
}

void personaje_destroy(t_personaje* self) {
	free(self->nombre);
	array_destroy(self->plan_de_niveles);
	dictionary_destroy_and_destroy_elements(self->objetivos,
			(void*) array_destroy);
	t_connection_destroy(self->orquestador);
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

t_nivel* nivel_create(t_connection_info* nivel, t_connection_info* planificador) {
	t_nivel* new = malloc(sizeof(t_nivel));
	new->data = nivel;
	new->planificador = planificador;
	return new;
}

void nivel_destroy(t_nivel* self) {
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
