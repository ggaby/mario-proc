#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "Nivel.h"
#include <commons/string.h>
#include <commons/collections/list.h>
#include "../common/common_structs.h"
#include "../common/list.h"

//"common_structs.h"

//Cambiar por el puerto del archivo de configuracion correspondiente
#define PORT 5001

bool verificar_argumentos(int argc, char* argv[]);
nivel_t_personaje* nivel_create_personaje(nivel_t_nivel* self,
		char id_personaje, t_socket_client* socket);
void cargar_recursos_config(t_config* config, nivel_t_nivel* new);
void nivel_create_grafica_recurso(nivel_t_nivel* self, t_recurso* recurso);
t_recurso* nivel_create_recurso(char* config_string);

int main(int argc, char *argv[]) {

	if (!verificar_argumentos(argc, argv)) {
		printf("Argumentos inválidos!\n");
		return EXIT_FAILURE;
	}

	nivel_t_nivel* self = nivel_create(argv[1]);

	if (!nivel_conectar_a_orquestador(self)) {
		nivel_destroy(self);
		return EXIT_FAILURE;
	}

	list_add(self->clients, self->socket_orquestador);

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

		if (mensaje->type != M_HANDSHAKE_PERSONAJE
				|| strcmp(mensaje->payload, PERSONAJE_HANDSHAKE) != 0) {
			mensaje_destroy(mensaje);
			sockets_destroyClient(client);
			return NULL ;
		}

		mensaje_destroy(mensaje);

		list_add(self->personajes, nivel_create_personaje(self, '@', client)); //TODO de donde sale el symbol??? :S
		responder_handshake(client, self->logger, NULL, "Nivel");

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

		//TODO hacer un case, por cada tipo de mensaje que manden los personajes y el orquestador_info

		switch (mensaje->type) {
		case M_GET_NOMBRE_NIVEL_REQUEST:
			nivel_get_nombre(self, client);
			break;
		case M_SOLICITUD_MOVIMIENTO:
			//nivel_mover_personaje(self, client, xdestino, ydestino);
			//TODO mover personaje.
		default:
			log_warning(self->logger,
					"Tipo del mensaje recibido no valido tipo: %d",
					mensaje->type);
			return false; //TODO WTF this FALSE!! hacer un send_error_message
		}

		mensaje_destroy(mensaje);
		return true;
	}

	sockets_create_little_server(NULL, self->puerto, self->logger, NULL,
			self->nombre, self->servers, self->clients, &acceptClosure,
			&recvClosure);

	while (true) {
		log_debug(self->logger, "Entro al select");
		mapa_dibujar(self->mapa);
		sockets_select(self->servers, self->clients, 0, &acceptClosure,
				&recvClosure);
	}

	log_info(self->logger, "%s: cerrado correctamente", self->nombre);

	nivel_destroy(self);

	return EXIT_SUCCESS;
}

nivel_t_nivel* nivel_create(char* config_path) {
	nivel_t_nivel* new = malloc(sizeof(nivel_t_nivel));
	t_config* config = config_create(config_path);
	new->nombre = string_duplicate(config_get_string_value(config, "Nombre"));
	new->tiempoChequeoDeadlock = config_get_int_value(config,
			"TiempoChequeoDeadlock");
	new->recovery = config_get_int_value(config, "Recovery");

	new->orquestador_info = connection_create(
			config_get_string_value(config, "orquestador"));

	char* log_file = string_duplicate("nivel.log");
	char* log_level = string_duplicate("INFO");

	if (config_has_property(config, "logFile")) {
		log_file = string_duplicate(config_get_string_value(config, "logFile"));
	}
	if (config_has_property(config, "logLevel")) {
		log_level = string_duplicate(
				config_get_string_value(config, "logLevel"));
	}

	new->logger = log_create(log_file, "Nivel", false,
			log_level_from_string(log_level));

	free(log_file);
	free(log_level);

	if (!config_has_property(config, "puerto")) {
		config_destroy(config);
		log_error(new->logger,
				"Error en archivo de configuracion: falta el puerto.");
		nivel_destroy(new);
		return NULL ;
	}

	new->puerto = config_get_int_value(config, "puerto");

	int cols = config_get_int_value(config, NIVEL_CONFIG_COLUMNAS);
	int rows = config_get_int_value(config, NIVEL_CONFIG_FILAS);

	new->mapa = mapa_create(rows, cols);

	cargar_recursos_config(config, new);

	config_destroy(config);

	new->socket_orquestador = NULL;
	new->clients = list_create();
	new->servers = list_create();

	log_info(new->logger, "Nivel  %s creado correctamente", new->nombre);
	return new;
}

void nivel_destroy(nivel_t_nivel* self) {
	free(self->nombre);
	connection_destroy(self->orquestador_info);
	log_destroy(self->logger);

	//TODO Free de los recursos

	if (self->socket_orquestador != NULL ) {
		bool is_socket_orquestador(t_socket_client* elem) {
			return sockets_equalsClients(self->socket_orquestador, elem);
		}

		my_list_remove_and_destroy_by_condition(self->clients,
				(void*) is_socket_orquestador, (void*) sockets_destroyClient);
	}

	mapa_destroy(self->mapa);

	list_destroy_and_destroy_elements(self->clients,
			(void*) sockets_destroyClient);
	list_destroy_and_destroy_elements(self->servers,
			(void*) sockets_destroyServer);

	free(self);
}

void nivel_get_nombre(nivel_t_nivel* self, t_socket_client* client) {
	t_mensaje* mensaje = mensaje_create(M_GET_NOMBRE_NIVEL_RESPONSE);
	mensaje_setdata(mensaje, string_duplicate(self->nombre),
			strlen(self->nombre) + 1);
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

bool nivel_conectar_a_orquestador(nivel_t_nivel* self) {
	self->socket_orquestador = sockets_conectar_a_servidor(NULL, self->puerto,
			self->orquestador_info->ip, self->orquestador_info->puerto,
			self->logger, M_HANDSHAKE_NIVEL, NIVEL_HANDSHAKE, HANDSHAKE_SUCCESS,
			"Orquestador");

	return (self->socket_orquestador != NULL );
}

nivel_t_personaje* nivel_create_personaje(nivel_t_nivel* self,
		char id_personaje, t_socket_client* socket) {
	nivel_t_personaje* personaje = malloc(sizeof(nivel_t_personaje));

	personaje->id = id_personaje;
	personaje->posX = 0;
	personaje->posY = 0;
	personaje->socket = socket;

	mapa_create_personaje(self->mapa, id_personaje);
	return personaje;
}

void nivel_mover_personaje(nivel_t_nivel* self, t_socket_client* client,
		int xdestino, int ydestino) {

	bool is_personaje(void* elem) {
		t_socket_client* socket_elem = ((nivel_t_personaje*) elem)->socket;
		return sockets_equalsClients(socket_elem, client);
	}

	nivel_t_personaje* personaje = list_find(self->personajes, &is_personaje);

	if (personaje->posX == xdestino && personaje->posY == ydestino) {
		return;
	}

	int distancia_a_mover = 1;
	int x = personaje->posX;
	int y = personaje->posY;

	//TODO hay logica repetida... despues veremos...
	if (x != xdestino) {
		if (x < xdestino) {
			x += distancia_a_mover;
		} else {
			x -= distancia_a_mover;
		}
	} else if (y != ydestino) {
		if (y < ydestino) {
			y += distancia_a_mover;
		} else {
			y -= distancia_a_mover;
		}
	}

	if (!mapa_mover_personaje(self->mapa, personaje->id, x, y)) {
		return;
	}

	mapa_dibujar(self->mapa);
	personaje->posX = x;
	personaje->posY = y;

}

void cargar_recursos_config(t_config* config, nivel_t_nivel* new) {

	int index = 1;
	char* key = string_from_format("Caja%d", index);

	if (new->recursos == NULL ) {
		new->recursos = list_create();
	}

	while (config_has_property(config, key)) {
		t_recurso* recurso = nivel_create_recurso(
				config_get_string_value(config, key));
		list_add(new->recursos, recurso);
		nivel_create_grafica_recurso(new, recurso);
		free(key);
		key = string_from_format("Caja%d", ++index);
	}

	free(key);
}

t_recurso* nivel_create_recurso(char* config_string) {
	//config_string example: Caja1=Flores,F,3,23,5
	char** values = string_split(config_string, ",");

	t_recurso* new_recurso = malloc(sizeof(t_recurso));
	new_recurso->nombre = string_duplicate(values[0]);
	new_recurso->simbolo = values[1][0]; // get first char of string.
	new_recurso->cantidad = atoi(values[2]);
	new_recurso->posX = atoi(values[3]);
	new_recurso->posY = atoi(values[4]);

	free(values[0]);
	free(values[1]);
	free(values[2]);
	free(values[3]);
	free(values[4]);
	free(values);

	return new_recurso;
}

void nivel_create_grafica_recurso(nivel_t_nivel* self, t_recurso* recurso) {
	mapa_create_caja_recurso(self->mapa, recurso->simbolo, recurso->posX,
			recurso->posY, recurso->cantidad);
}
