/*
 * Plataforma.c
 *
 *  Created on: 11/05/2013
 *      Author: reyiyo
 */

#include <stdlib.h>
#include <pthread.h>
#include "Orquestador/Orquestador.h"

void* orquestador(void* name);

int main(int argc, char* argv[]) {

	pthread_t thread_orquestador;
	char* name = "Orquestador";
	pthread_create(&thread_orquestador, NULL, orquestador, (void*) name);
	pthread_join(thread_orquestador, NULL );
	return EXIT_SUCCESS;
}


