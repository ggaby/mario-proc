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
#include "../common/sockets.h"

#define PORT 5001

//t_nivel* orquestador_get_info_nivel(int socket_orquestador,
//		char** plan_de_niveles);
t_socket_client* crear_conexion_a_orquestador(t_connection_info* orquestador);
t_socket_client* personaje_conectar_a_orquestador(t_personaje* self);
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

	t_socket_client* orquestador = personaje_conectar_a_orquestador(self);
	printf("Conectado con la plataforma\n");
	sockets_sendString(orquestador, "Aquí un personaje reportándose!");
//	personaje_conectar_a_nivel(t_nivel);
	personaje_destroy(self);
	sockets_destroyClient(orquestador);
	printf("Conexión terminada\n");
	return EXIT_SUCCESS;
}


t_socket_client* personaje_conectar_a_orquestador(t_personaje* self) {
	t_socket_client* orquestador = crear_conexion_a_orquestador(self->orquestador);
	return orquestador;
	//return orquestador_get_info_nivel(socket_orquestador, self->plan_de_niveles);
}

t_socket_client* crear_conexion_a_orquestador(t_connection_info* orquestador) {
	t_socket_client* client = sockets_createClient(NULL, PORT);
	sockets_connect(client, orquestador->ip, orquestador->puerto);
	return client;
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
