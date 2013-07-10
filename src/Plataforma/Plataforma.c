/*
 * Plataforma.c
 *
 *  Created on: 11/05/2013
 *      Author: reyiyo
 */

#include <stdlib.h>
#include "Plataforma.h"
#include "Orquestador/Orquestador.h"
#include "Planificador/Planificador.h"
#include <commons/string.h>

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
	pthread_mutex_init(&new->logger_mutex, NULL );
	new->niveles = list_create();

	new->config_path = config_path;
	//FIXME no hago un duplicate porque sino tengo que hacer free del param, si asigno el puntero no desperdicio memoria, por ende no hace falta hacer free.. es correcto?
	return new;
}

void plataforma_nivel_destroy(plataforma_t_nivel* nivel) {
	free(nivel->nombre);
	sockets_destroyClient(nivel->socket_nivel);
	t_connection_destroy(nivel->planificador);
	free(nivel);
}

void plataforma_destroy(t_plataforma* self) {
	log_destroy(self->logger);
	pthread_mutex_destroy(&self->logger_mutex);
	list_destroy_and_destroy_elements(self->niveles,
			(void*) plataforma_nivel_destroy);
	free(self);
}

int plataforma_create_nivel(t_plataforma* self, char* nombre_nivel,
		t_socket_client* socket_nivel, char* planificador_connection_info) {

	plataforma_t_nivel* new = malloc(sizeof(plataforma_t_nivel));

	new->nombre = string_duplicate(nombre_nivel);
	new->socket_nivel = socket_nivel;
	new->planificador = t_connection_create(planificador_connection_info);

	thread_planificador_args* args = malloc(sizeof(thread_planificador_args));
	args->plataforma = self;
	args->nivel = new;

	//TODO VER DONDE HACER EL FREE de args (al iniciar planificador?)
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

