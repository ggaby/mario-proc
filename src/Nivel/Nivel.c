#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "Nivel.h"
#include <commons/string.h>

//Cambiar por el puerto del archico de configuracion correspondiente
#define PORT 5001

bool verificar_argumentos(int argc, char* argv[]);

int main (int argc,char *argv[]){

	if(!verificar_argumentos(argc, argv)){
		printf("argumentos invalidos!\n");
		return EXIT_FAILURE;
	}

	nivel_t_nivel* self;
	self = nivel_create(argv[1]);

	t_socket_client* socket_orquestador = nivel_conectar_a_orquestador(self);
	if(socket_orquestador == NULL){
		nivel_destroy(self);
		return EXIT_FAILURE;
	}
	
	//Se crea un socket para escuchar conexiones

	t_socket_server* server = sockets_createServer(NULL, self->puerto);

	if (!sockets_listen(server)) {
		printf("No se puede escuchar\n");
		sockets_destroyServer(server);
		sockets_destroyClient(socket_orquestador);
		nivel_destroy(self);
		return EXIT_FAILURE;
	}

	log_info(self->logger, "Escuchando conexiones entrantes en 127.0.0.1:%d", self->puerto);

	t_list* servers = list_create();
	list_add(servers, server);

	//closure que maneja las nuevas conexiones
	t_socket_client* acceptClosure(t_socket_server* server) {
		t_socket_client* client = sockets_accept(server);
		t_socket_buffer* buffer = sockets_recv(client);

		if (buffer == NULL ) {
			sockets_destroyClient(client);
			log_warning(self->logger, "Error al recibir datos en el accept");
			return NULL ;
		}

		t_mensaje* mensaje = mensaje_deserializer(buffer, 0);
		sockets_bufferDestroy(buffer);

		if (mensaje->type != M_HANDSHAKE_PERSONAJE || strcmp(mensaje->payload, PERSONAJE_HANDSHAKE) != 0) {
			mensaje_destroy(mensaje);
			sockets_destroyClient(client);
			return NULL ;
		}

		mensaje_destroy(mensaje);
		log_info(self->logger, "Personaje conectado por el socket %d", client->socket->desc);

		//TODO ESTE BLOQUE ES IGUAL AL RESPONDER HANDSHAKE DE ORQUESTADOR
		mensaje = mensaje_create(M_HANDSHAKE_RESPONSE);
		mensaje_setdata(mensaje, strdup(HANDSHAKE_SUCCESS),
				strlen(HANDSHAKE_SUCCESS) + 1);
		mensaje_send(mensaje, client);
		mensaje_destroy(mensaje);
		//----

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

		//TODO hacer un case, por cada tipo de mensaje que manden los personajes y el orquestador

		switch(mensaje->type){
			case M_GET_NOMBRE_NIVEL_REQUEST:
				nivel_get_nombre(self, client);
				break;
			default:
				log_warning(self->logger, "Tipo del mensaje recibido no valido tipo: %d", mensaje->type);
				return false; //TODO WTF this FALSE!! hacer un send_error_message
		}

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

nivel_t_nivel* nivel_create(char* config_path){
	nivel_t_nivel* new = malloc(sizeof(nivel_t_nivel));
	t_config* config = config_create(config_path);
	new->nombre = string_duplicate(config_get_string_value(config, "Nombre"));
	new->tiempoChequeoDeadlock = config_get_int_value(config, "TiempoChequeoDeadlock");
	new->recovery = config_get_int_value(config, "Recovery");

	new->orquestador = connection_create(config_get_string_value(config, "orquestador"));

	//TODO Ver como se cargan los recursos

	char* log_file = string_duplicate("nivel.log");
	char* log_level = string_duplicate("INFO");

	if (config_has_property(config, "logFile")) {
		log_file = string_duplicate(config_get_string_value(config, "logFile"));
	}
	if (config_has_property(config, "logLevel")) {
		log_level = string_duplicate(config_get_string_value(config, "logLevel"));
	}

	new->logger = log_create(log_file, "Nivel", true, log_level_from_string(log_level));

	free(log_file);
	free(log_level);

	if (!config_has_property(config, "puerto")) {
		config_destroy(config);
		log_error(new->logger, "Error en archivo de configuracion: falta el puerto.");
		nivel_destroy(new);
		return NULL ;
	}

	new->puerto = config_get_int_value(config, "puerto");

	config_destroy(config);

	log_info(new->logger, "Nivel  %s creado correctamente", new->nombre);
	return new;
}

void nivel_destroy(nivel_t_nivel* self) {
	free(self->nombre);
	connection_destroy(self->orquestador);
	log_destroy(self->logger);
	//TODO Free de los recursos
	free(self);
}

void nivel_get_nombre(nivel_t_nivel* self, t_socket_client* client){
	t_mensaje* mensaje = mensaje_create(M_GET_NOMBRE_NIVEL_RESPONSE);
	mensaje_setdata(mensaje, string_duplicate(self->nombre), strlen(self->nombre)+1);
	mensaje_send(mensaje, client);
	mensaje_destroy(mensaje);
}

bool verificar_argumentos(int argc, char* argv[]) {
	//TODO Validar parametros: ver si no es igual al de personaje para no repetir codigo. Tal vez se puede parametrizar la cantidad de args.
	if (argc < 2) {
		printf("Error en la cantidad de argumentos.\n");
		return false;
	}
	return true;
}

//TODO esta funcion se puede extraer a la lib de sockets (ver que comparte to-do con el de personaje, hay que parametrizarla con (t_conection_info, t_log, int puerto)
t_socket_client* nivel_conectar_a_orquestador(nivel_t_nivel* self){

	//Se crea un socket que se conecta al orquestador,

	t_socket_client* socket_orquestador=sockets_createClient(NULL, self->puerto);

	if(socket_orquestador == NULL)
	{
		log_error(self->logger, "Error al crear el socket para conectarse con el orquestador");
		return NULL;
	}

	//Si no puede conectarse al orquestador,
	//se borra el socket que se creo
	if(sockets_connect(socket_orquestador,self->orquestador->ip, self->orquestador->puerto) == 0)
	{
		sockets_destroyClient(socket_orquestador);
		return NULL;
	}


	//Se crear un mensaje para enviar el HANDSHAKE

	t_mensaje* mensaje = mensaje_create(M_HANDSHAKE_NIVEL);
	mensaje_setdata(mensaje, strdup(NIVEL_HANDSHAKE), strlen(NIVEL_HANDSHAKE) + 1);

	log_info(self->logger, "Enviando handshake al orquestador");

	mensaje_send(mensaje, socket_orquestador);
	mensaje_destroy(mensaje);

	//Espera recibir una respuesta del orquestador.
	t_socket_buffer* buffer = sockets_recv(socket_orquestador);
	log_info(self->logger, "Recibiendo resultado del handshake");

	if (buffer == NULL ) {
		log_info(self->logger, "Error en el resultado del handshake");
		sockets_destroyClient(socket_orquestador);
		return NULL;
	}

	t_mensaje* rta_handshake = mensaje_deserializer(buffer, 0);
	sockets_bufferDestroy(buffer);

	if (rta_handshake->length != (strlen(HANDSHAKE_SUCCESS) + 1)
				|| (!string_equals_ignore_case((char*) rta_handshake->payload, HANDSHAKE_SUCCESS))) {

		log_error(self->logger, "Error en la respuesta del handshake");
		mensaje_destroy(rta_handshake);
		sockets_destroyClient(socket_orquestador);
		return NULL;
	}

	mensaje_destroy(rta_handshake);

	log_info(self->logger, "Conectado con el Orquestador: Origen: 127.0.0.1:%d, Destino: %s:%d",
				self->puerto, self->orquestador->ip, self->orquestador->puerto);

	return socket_orquestador;

}
