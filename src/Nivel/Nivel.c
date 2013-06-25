#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "Nivel.h"
#include "../common/sockets.h"
#include <commons/string.h>

//Cambiar por el puerto del archico de configuracion correspondiente
#define PORT 5001

t_nivel* self;

int main (int argc,char *argv[])
{
	//TODO Validar parametros
	self = nivel_create(argv[1]);

	//Se crea un socket que se conecta a plataforma,
	//en nivel en este caso es cliente
	t_socket_client* socket_orquestador=sockets_createClient(NULL, PORT);
	printf("Nivel creado\n");

	//Si no se conecta, devolver error,
	//para luego guardar en el log
	if(socket_orquestador == NULL)
	{
		nivel_destroy(self);
		return EXIT_FAILURE;
	}

	//Si no puede conectarse al orquestador,
	//se borra el socket que se creo
	if(sockets_connect(socket_orquestador,self->orquestador->ip, self->orquestador->puerto) == 0)
	{
		sockets_destroyClient(socket_orquestador);
		nivel_destroy(self);
		return EXIT_FAILURE;
	}
			
			
	//Se crear un mensaje para enviar al HANDSHAKE
	//LA LOGICA ES PRIMERO SE COMUNICA CON EL ORQUESTADOR
	//LUEGO SE PONE A ESCUCHAR PERSONAJES
	t_mensaje* mensaje = mensaje_create(M_HANDSHAKE);
	mensaje_setdata(mensaje, strdup(NIVEL_HANDSHAKE),
			strlen(NIVEL_HANDSHAKE) + 1);

	printf("DATA: %s\n", (char*) mensaje_getdata(mensaje));
	mensaje_send(mensaje, socket_orquestador);
	mensaje_destroy(mensaje);
	//Se termina de usar los mensajes


	//Se bloquea el socket orquestador hasta recibir una respuesta
	t_socket_buffer* buffer = sockets_recv(socket_orquestador);
	printf("Recibiendo resultado del handshake\n");
	
	if (buffer == NULL ) {
		printf("Error en el resultado\n");
		sockets_destroyClient(socket_orquestador);
		nivel_destroy(self);
		return EXIT_FAILURE;
	}

	t_mensaje* rta_handshake = mensaje_deserializer(buffer, 0);
	sockets_bufferDestroy(buffer);

	if (rta_handshake->length != (strlen("OK") + 1)//se compara por el tipo
			|| (!string_equals_ignore_case((char*) rta_handshake->payload, "OK"))) {
		printf("Error en el LENGTH del resultado\n");
		mensaje_destroy(rta_handshake);
		sockets_destroyClient(socket_orquestador);
		nivel_destroy(self);
		return EXIT_FAILURE;
	}

	printf("TYPE: %d\n", rta_handshake->type);
	printf("LENGHT: %d\n", rta_handshake->length);
	printf("DATA: %s\n", (char*) rta_handshake->payload);
	mensaje_destroy(rta_handshake);

	printf("Conectado a la Plataforma\n");

	//Se crea un socket para escuchar conexiones

	t_socket_server* server = sockets_createServer(NULL, PORT);

	if (!sockets_listen(server)) {
		printf("No se puede escuchar\n");
		sockets_destroyServer(server);
		sockets_destroyClient(socket_orquestador);
		nivel_destroy(self);
		return EXIT_FAILURE;
	}

	printf("Escuchando conexiones entrantes.\n");

	t_list *servers = list_create();
	list_add(servers, server);

	//closure que maneja las nuevas conexiones
	t_socket_client* acceptClosure(t_socket_server* server) {
		t_socket_client* client = sockets_accept(server);
		t_socket_buffer* buffer = sockets_recv(client);

		if (buffer == NULL ) {
			sockets_destroyClient(client);
			printf("Error al recibir datos en el accept\n");
			return NULL ;
		}

		t_mensaje* mensaje = mensaje_deserializer(buffer, 0);
		sockets_bufferDestroy(buffer);
		if (!mensaje_validar_handshake(client, mensaje, NIVEL_HANDSHAKE)) {
			mensaje_destroy(mensaje);
			sockets_destroyClient(client);
			return NULL ;
		}

		mensaje_destroy(mensaje);
		return client;
	}
	
	//closure que maneja los mensajes recibidos
	int recvClosure(t_socket_client* client) {
		t_socket_buffer* buffer = sockets_recv(client);

		if (buffer == NULL ) {
			return false;
		}

		t_mensaje* mensaje = mensaje_deserializer(buffer, 0);
		sockets_bufferDestroy(buffer);

		//TODO: hacer un case, por cada tipo de mensaje que manden los personajes y el orquestador

		mensaje_destroy(mensaje);
		return true;
	}
	
	t_list* clients = list_create();
	list_add(clients, socket_orquestador);

	while (true) {
		printf("Entro al select\n");
		sockets_select(servers, clients, 0, &acceptClosure, &recvClosure);
	}

	list_destroy_and_destroy_elements(servers, (void*) sockets_destroyServer);
	list_destroy_and_destroy_elements(clients, (void*) sockets_destroyClient);

	printf("Proceso Nivel: %s cerrado correctamente.\n", self->nombre);

	nivel_destroy(self);

	return EXIT_SUCCESS;
}

t_nivel* nivel_create(char* config_path){
	t_nivel* new = malloc(sizeof(t_nivel));
	t_config* config = config_create(config_path);
	new->nombre = string_duplicate(config_get_string_value(config, "Nombre"));
	new->tiempoChequeoDeadlock = config_get_int_value(config, "TiempoChequeoDeadlock");
	new->recovery = config_get_int_value(config, "Recovery");

	new->orquestador = t_connection_new(
				config_get_string_value(config, "orquestador"));

	//TODO Ver como se cargan los recursos

	config_destroy(config);
	return new;
}

void nivel_destroy(t_nivel* self) {
	free(self->nombre);
	t_connection_destroy(self->orquestador);
	//TODO Free de los recursos
	free(self);
}
