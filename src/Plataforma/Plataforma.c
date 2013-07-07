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
	new->niveles = list_create();
	return new;
}

void plataforma_nivel_destroy(plataforma_t_nivel* nivel){
	free(nivel->nombre);
	sockets_destroyClient(nivel->socket_nivel);
	t_connection_destroy(nivel->planificador);
	free(nivel);
}

void plataforma_destroy(t_plataforma* self) {
	log_destroy(self->logger);
	pthread_mutex_destroy(&self->logger_mutex);
	list_destroy_and_destroy_elements(self->niveles, (void*)plataforma_nivel_destroy);
	free(self);
}


plataforma_t_nivel* plataforma_create_add_nivel(t_plataforma* self, char* nombre_nivel, t_socket_client* socket_nivel, char* planificador_connection_info){

	plataforma_t_nivel* new_nivel = malloc(sizeof(plataforma_t_nivel));

	new_nivel->nombre = string_duplicate(nombre_nivel);
	new_nivel->socket_nivel = socket_nivel;
	new_nivel->planificador = t_connection_new(planificador_connection_info);
	list_add(self->niveles, new_nivel);

	return new_nivel;

}

