/*
 * Plataforma.c
 *
 *  Created on: 11/05/2013
 *      Author: reyiyo
 */

#include <stdlib.h>
#include "Orquestador/Orquestador.h"
#include "Plataforma.h"
#include <commons/string.h>

int main(int argc, char* argv[]) {

	t_plataforma* plataforma = plataforma_create();

	pthread_t thread_orquestador;
	pthread_create(&thread_orquestador, NULL, orquestador, (void*) plataforma);
	pthread_join(thread_orquestador, NULL );

	plataforma_destroy(plataforma);
	return EXIT_SUCCESS;
}

t_plataforma* plataforma_create() {
	t_plataforma* new = malloc(sizeof(t_plataforma));
	new->logger = log_create("plataforma.log", "Plataforma", true,
			log_level_from_string("TRACE"));
	pthread_mutex_init(&new->logger_mutex, NULL);
	return new;
}

void plataforma_destroy(t_plataforma* self) {
	log_destroy(self->logger);
	pthread_mutex_destroy(&self->logger_mutex);
	free(self);
}
