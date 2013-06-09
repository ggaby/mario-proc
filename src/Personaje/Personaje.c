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
#include "../common/mensaje.h"
#include <commons/string.h>

#define PORT 5001

void personaje_perder_vida(int n);

t_personaje* self;

int main(int argc, char* argv[]) {
	//TODO verificar argumentos or else
//	if (!verificar_argumentos(argv)) {
//		printf("Error en la cantidad de argumentos\n");
//		return EXIT_FAILURE;
//	}

	self = personaje_create(argv[1]);
	signal(SIGUSR1, &personaje_perder_vida);
	printf("Personaje creado\n");

	t_socket_client* socket_orquestador = sockets_createClient(NULL, PORT);

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

	personaje_destroy(self);
	sockets_destroyClient(socket_orquestador);
	printf("Conexión terminada\n");
	return EXIT_SUCCESS;
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
