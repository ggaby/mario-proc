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
#include "structs.h"

t_nivel* orquestador_get_info_nivel(int socket_orquestador,
		char** plan_de_niveles);
int crear_conexion_a_orquestador(t_connection_info* orquestador);
t_nivel* personaje_conectar_a_orquestador(t_personaje* self);
void personaje_perder_vida(int n);

t_personaje* self;
int main(int argc, char* argv[]) {
	//TODO verificar argumentos or else
//	if (!verificar_argumentos(argv)) {
//		printf("Error en la cantidad de argumentos\n");
//		return EXIT_FAILURE;
//	}

	self = personaje_new(argv[1]);
	signal(SIGUSR1, personaje_perder_vida);
	printf("Personaje creado\n");

	t_nivel* nivel = personaje_conectar_a_orquestador(self);
//	personaje_conectar_a_nivel(t_nivel);
	printf("Nivel creado:\n%s\n%s\n", nivel->data->ip, nivel->planificador->ip);
	personaje_destroy(self);
	nivel_destroy(nivel);
	return EXIT_SUCCESS;
}

void personaje_perder_vida(int n) {
	printf("Perdiendo vida!\n");
	switch (n) {
	case SIGINT:
		printf("No salgo nada... te cabió\n");
		break;
	case SIGUSR1:
		self->vidas--;
		printf("Ahora me quedan %d vidas\n", self->vidas);
		fflush(stdout);
		break;
	}
}

t_nivel* personaje_conectar_a_orquestador(t_personaje* self) {
	int socket_orquestador = crear_conexion_a_orquestador(self->orquestador);
	return orquestador_get_info_nivel(socket_orquestador, self->plan_de_niveles);
}

int crear_conexion_a_orquestador(t_connection_info* orquestador) {
//TODO
	/*
	 * CONECTARSE POR SOCKETS UTILIZANDO LIBRERIA COMUN Y TODA LA BOLA
	 */
//return socket
	return 0;
}

t_nivel* orquestador_get_info_nivel(int socket_orquestador,
		char** plan_de_niveles) {
	//TODO
		/*
		 * CONECTARSE POR SOCKETS UTILIZANDO LIBRERIA COMUN Y TODA LA BOLA
		 * ENVIAR MENSAJE DE PEDIDO DE INFO DE PROXIMO NIVEL
		 */
	//return nivel;
	t_connection_info* nivel = t_connection_new("123.456.789.012:5000");
	t_connection_info* planificador = t_connection_new("123.456.789.013:5001");
	t_nivel* nivel_de_prueba = nivel_new(nivel, planificador);
	return nivel_de_prueba;
}
