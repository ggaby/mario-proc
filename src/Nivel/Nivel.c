#include <stdio.h>
#include <stdlib.h>
#include <signail.h>
#include <unistd.h>
#include "nivel.h"
#include "../common/socket.h"
#include <commons/string.h>

//Cambiar por el puerto del archico de configuracion correspondiente
#define PORT 5001
int main (int argc,char *argv[])
{
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

	//Si se pierde la coneccion a plataforma,
	//se borra el socket que se creo
	if(sockets_connect(socket_orquestador,self->orquestador->ip, self->orquestador->port) == 0)
	{
		socket_destroyClient(socket_orquestador);
		nivel_destroy(self);
		return EXIT_FAILURE;
	}
			
			
	//Se crear un mensaje para enviar al HANDSHAKE
	//LA LOGICA ES PRIMERO SE COMUNICA CON EL ORQUESTADOR
	//LUEGO SE PONE A ESCUCHAR PERSONAJES
	t_mensaje* mensaje = mensaje_create(M_HANDSHAKE);
	mensaje_setdata(mensaje, strdup(DATA_NIVEL_HANDSHAKE),
			strlen(DATA_NIVEL_HANDSHAKE) + 1);
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

	//Si el mensaje que recibe no se destrullo porque es NULL,
	//se para hacer la accion
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



	//Se termina de conectarse a plataforma el proceso nivel en este caso es cliente---------------------

	


	//-------------Se crea un socket nivel que en este caso es servidor------------------------

			
	//Se empieza a recibir personajes
	t_socket_server* server = sockets_createServer(NULL, PORT);
	printf("Conectado con el orquestador\n");

	if (!sockets_listen(server)) {
		printf("No se puede escuchar\n");
		sockets_destroyServer(server);
		return EXIT_FAILURE;
	}

	printf("Escuchando conexiones entrantes.\n");

	t_list *servers = list_create();
	list_add(servers, server);

	//Se aceptan el mensajes del servidor
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
		if (!handshake(client, mensaje)) {
			mensaje_destroy(mensaje);
			sockets_destroyClient(client);
			return NULL ;
		}

		mensaje_destroy(mensaje);
		return client;
	}
	
	//Se bloquea hasta recibir un mensaje del cliente, los personajes
	int recvClosure(t_socket_client* client) {
		t_socket_buffer* buffer = sockets_recv(client);

		if (buffer == NULL ) {
			return false;
		}

		t_mensaje* mensaje = mensaje_deserializer(buffer, 0);
		//TODO: hacer un case, por cada tipo de mensaje que manden los personajes y el orquestador
		mostrar_mensaje(mens:je, client);

		mensaje_destroy(mensaje);
		sockets_bufferDestroy(buffer);
		return true;
	}
	
	t_list* clients = list_create();

	while (true) {
		printf("Entro al select\n");
		sockets_select(servers, clients, 0, &acceptClosure, &recvClosure);
	}

	list_destroy_and_destroy_elements(servers, (void*) sockets_destroyServer);
	list_destroy_and_destroy_elements(clients, (void*) sockets_destroyClient);
	printf("Server cerrado correctamente.\n");
	//Se termina a recibir personajes, el nivel en este caso es servidor

	//nivel_destroy(self);
	//sockets_destroyClient(socket_orquestador);
	//printf("Conexion terminada\n");
	return EXIT_SUCCESS;
}

t_nivel* nivel_create(char* config_path){
	t_nivel* new = malloc(sizeof(t_nivel));
	t_config* config = config_create(config_path);
	new->nombre = string_duplicate(config_get_string_value(config, "nombre"));
	//TODO Ver como se cargan los recursos
	new->recovery = config_get_int_value(config, "recovery");
	config_destroy(config);
	free(s);
	return new;
}

void nivel_destroy(t_nivel* self) {
	t_connection_destroy(self->data);
	t_connection_destroy(self->planificador);
	free(self);
}
