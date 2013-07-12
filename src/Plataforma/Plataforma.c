/*
 * Plataforma.c
 *
 *  Created on: 11/05/2013
 *      Author: reyiyo
 */

#include <stdlib.h>
#include "Plataforma.h"
#include <commons/string.h>
#include "Orquestador/Orquestador.h"
#include "Planificador/Planificador.h"
#include <commons/config.h>

int main(int argc, char* argv[]) {

	//TODO validar Args

	t_plataforma* plataforma = plataforma_create(argv[1]);

	pthread_t thread_orquestador;
	pthread_create(&thread_orquestador, NULL, orquestador, (void*) plataforma);
	pthread_join(thread_orquestador, NULL );

	plataforma_destroy(plataforma);
	return EXIT_SUCCESS;
}

t_plataforma* plataforma_create(char* config_path) {
	t_plataforma* new = malloc(sizeof(t_plataforma));
	new->logger = log_create("plataforma.log", "Plataforma", true,
			log_level_from_string("TRACE"));
	pthread_mutex_init(&new->logger_mutex, NULL);
	new->niveles = list_create();
	new->config_path = string_duplicate(config_path);

	t_config* config = config_create(config_path);
	new->ip = string_duplicate(config_get_string_value(config, "ip"));
	config_destroy(config);

	return new;
}

void plataforma_destroy(t_plataforma* self) {
	log_destroy(self->logger);
	pthread_mutex_destroy(&self->logger_mutex);
	list_destroy_and_destroy_elements(self->niveles,
			(void*) plataforma_nivel_destroy);
	free(self->config_path);

	if (self->orquestador != NULL ) {
		orquestador_destroy(self->orquestador);
	}
	free(self);
}

int plataforma_create_nivel(t_plataforma* self, char* nombre_nivel,
		t_socket_client* socket_nivel, char* planificador_connection_info) {

	plataforma_t_nivel* new = malloc(sizeof(plataforma_t_nivel));

	new->nombre = string_duplicate(nombre_nivel);
	new->socket_nivel = socket_nivel;

	thread_planificador_args* args = malloc(sizeof(thread_planificador_args));
	args->plataforma = self;
	args->nombre_nivel = string_duplicate(nombre_nivel);
	args->planificador_connection_info = connection_create(
			planificador_connection_info);

	if (pthread_create(&new->thread_planificador, NULL, planificador,
			(void*) args) != 0) {
		plataforma_nivel_destroy(new);
		pthread_mutex_lock(&self->logger_mutex);
		log_error(self->logger,
				"Plataforma: No se pudo crear el thread planificador del nivel %s",
				nombre_nivel);
		pthread_mutex_unlock(&self->logger_mutex);
		return 1;
	}

	pthread_mutex_lock(&self->logger_mutex);
	log_debug(self->logger, "Plataforma: Planificador para el nivel %s creado",
			nombre_nivel);
	pthread_mutex_unlock(&self->logger_mutex);

	list_add(self->niveles, new);

	return 0;
}

void plataforma_nivel_destroy(plataforma_t_nivel* nivel) {
	free(nivel->nombre);
	sockets_destroyClient(nivel->socket_nivel);
	connection_destroy(nivel->connection_info);
	free(nivel);
}

plataforma_t_nivel* plataforma_get_nivel(t_plataforma* self, char* nombre_nivel) {
	bool mismo_nombre(plataforma_t_nivel* elem) {
		return string_equals_ignore_case(elem->nombre, nombre_nivel);
	}

	return list_find(self->niveles, (void*) mismo_nombre);
}
