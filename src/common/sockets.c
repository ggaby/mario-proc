#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <time.h>
#include <netinet/tcp.h>
#include "sockets.h"
#include <commons/collections/list.h>
#include "errno.h"
#include "list.h"
#include "mensaje.h"
#include <commons/string.h>

/*
 *						Socket Buffer Functions
 */

void sockets_bufferDestroy(t_socket_buffer *tbuffer) {
	free(tbuffer);
}

void sockets_sbufferDestroy(t_socket_sbuffer *tbuffer) {
	free(tbuffer->serializated_data);
	free(tbuffer);
}

/*
 *						Private Low Level Socket Functions
 */

void *sockets_makeaddr(char* ip, int port);
t_socket *sockets_create(char* ip, int port);
int sockets_bind(t_socket* sckt, char* ip, int port);
void sockets_destroy(t_socket* sckt);

void *sockets_makeaddr(char* ip, int port) {
	struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	if (ip == NULL ) {
		addr->sin_addr.s_addr = INADDR_ANY;
	} else {
		addr->sin_addr.s_addr = inet_addr(ip);
	}
	addr->sin_port = htons(port);
	return addr;
}

t_socket *sockets_create(char* ip, int port) {
	t_socket* sckt = malloc(sizeof(t_socket));
	sckt->desc = socket(AF_INET, SOCK_STREAM, 0);
	int flag = 1;
	setsockopt(sckt->desc, SOL_TCP, TCP_NODELAY, &flag, sizeof(flag));
	if (!sockets_bind(sckt, ip, port)) {
		free(sckt);
		return NULL ;
	}
	sockets_setMode(sckt, SOCKETMODE_BLOCK);
	return sckt;
}

int sockets_bind(t_socket* sckt, char* ip, int port) {
	int yes = 1;
	sckt->my_addr = sockets_makeaddr(ip, port);

	if (setsockopt(sckt->desc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) {
		free(sckt->my_addr);
		sckt->my_addr = NULL;
		return 0;
	}

	if (bind(sckt->desc, (struct sockaddr *) sckt->my_addr,
			sizeof(struct sockaddr_in)) == -1) {
		free(sckt->my_addr);
		sckt->my_addr = NULL;
		return 0;
	}
	return 1;
}

void sockets_destroy(t_socket* sckt) {
	if (sckt->desc > 0) {
		close(sckt->desc);
	}
	free(sckt->my_addr);
	free(sckt);
}

/*
 *						Public Low Level Socket Functions
 */

char* sockets_getIp(t_socket* sckt) {
	return inet_ntoa(((struct sockaddr_in *) sckt->my_addr)->sin_addr);
}

void sockets_getIpAsBytes(t_socket* sckt, unsigned char ip[]) {
	sockets_getStringIpAsBytes(
			inet_ntoa(((struct sockaddr_in *) sckt->my_addr)->sin_addr), ip);
}

void sockets_getStringIpAsBytes(char* str_ip, unsigned char ip[]) {
	int ip_lenght = strlen(str_ip);
	int cont, index_ip = 0;

	ip[index_ip] = 0;
	for (cont = 0; cont < ip_lenght; cont++) {
		if (str_ip[cont] == '.') {
			index_ip++;
			ip[index_ip] = 0;
		} else {
			ip[index_ip] = (ip[index_ip] * 10) + (str_ip[cont] - '0');
		}
	}
}

int sockets_getPort(t_socket* sckt) {
	return htons(((struct sockaddr_in *) sckt->my_addr)->sin_port);
}

void sockets_setMode(t_socket *sckt, e_socket_mode mode) {
	sckt->mode = mode;
}

e_socket_mode sockets_getMode(t_socket *sckt) {
	return sckt->mode;
}

int sockets_isBlocked(t_socket *sckt) {
	return sckt->mode == SOCKETMODE_BLOCK;
}

/*
 *						Client Socket Functions
 */

t_socket_client* sockets_createClient(char *ip, int port) {
	t_socket_client* client = malloc(sizeof(t_socket_client));
	if ((client->socket = sockets_create(ip, port)) == NULL ) {
		free(client);
		return NULL ;
	}
	if (client->socket == NULL ) {
		return NULL ;
	}
	sockets_setState(client, SOCKETSTATE_DISCONNECTED);
	return client;
}

t_socket *sockets_getClientSocket(t_socket_client *client) {
	return client->socket;
}

int sockets_equalsClients(t_socket_client *client1, t_socket_client *client2) {
	return client1->socket->desc == client2->socket->desc;
}

int sockets_isConnected(t_socket_client *client) {
	return client->state == SOCKETSTATE_CONNECTED;
}

e_socket_state sockets_getState(t_socket_client *client) {
	return client->state;
}

void sockets_setState(t_socket_client *client, e_socket_state state) {
	client->state = state;
}

int sockets_connect(t_socket_client *client, char *server_ip, int server_port) {
	client->serv_socket = malloc(sizeof(t_socket));
	client->serv_socket->my_addr = sockets_makeaddr(server_ip, server_port);

	if (connect(client->socket->desc,
			(struct sockaddr *) client->serv_socket->my_addr,
			sizeof(struct sockaddr_in)) == -1) {
		sockets_destroy(client->serv_socket);
		client->serv_socket = NULL;
		return 0;
	}
	sockets_setState(client, SOCKETSTATE_CONNECTED);
	return 1;
}

int sockets_send(t_socket_client *client, void *data, int datalen) {
	int aux = send(client->socket->desc, data, datalen, 0);
	if (aux == -1)
		sockets_setState(client, SOCKETSTATE_DISCONNECTED);

	return aux;
}

int sockets_write(t_socket_client *client, void *data, int datalen) {
	return write(client->socket->desc, data, datalen);
}

int sockets_sendBuffer(t_socket_client *client, t_socket_buffer *buffer) {
	return sockets_send(client, buffer->data, buffer->size);
}

int sockets_sendSBuffer(t_socket_client *client, t_socket_sbuffer *buffer) {
	return sockets_send(client, buffer->serializated_data, buffer->size);
}

int sockets_sendString(t_socket_client *client, char *str) {
	return sockets_send(client, (void*) str, strlen(str) + 1);
}

int sockets_sendSerialized(t_socket_client *client, void *data,
		t_socket_sbuffer *(*serializer)(void*)) {
	t_socket_sbuffer *sbuffer = serializer(data);
	if (sbuffer != NULL ) {
		int ret = sockets_send(client, (void*) sbuffer->serializated_data,
				sbuffer->size);
		free(sbuffer->serializated_data);
		free(sbuffer);
		return ret;
	}
	return -1;
}

void *sockets_recvSerialized(t_socket_client *client,
		void *(*deserializer)(t_socket_sbuffer*)) {
	t_socket_buffer *buffer = sockets_recv(client);
	void *data = NULL;

	if (buffer != NULL ) {
		t_socket_sbuffer sbuffer;
		sbuffer.size = buffer->size;
		sbuffer.serializated_data = malloc(buffer->size);
		memcpy(sbuffer.serializated_data, buffer->data, buffer->size);
		data = deserializer(&sbuffer);
		free(sbuffer.serializated_data);
		free(buffer);
	}
	return data;
}

t_socket_buffer *sockets_recv(t_socket_client *client) {
	t_socket_buffer *tbuffer = malloc(sizeof(t_socket_buffer));
	int datasize = sockets_recvInBuffer(client, tbuffer);

	if (datasize <= 0) {
		free(tbuffer);
		return NULL ;
	}

	return tbuffer;
}

int sockets_recvInBuffer(t_socket_client *client, t_socket_buffer *buffer) {
	memset(buffer->data, 0, DEFAULT_BUFFER_SIZE);
	if (!sockets_isBlocked(client->socket)) {
		fcntl(client->socket->desc, F_SETFL, O_NONBLOCK);
	}
	buffer->size = recv(client->socket->desc, buffer->data, 3,
			MSG_PEEK | MSG_WAITALL);
	uint16_t size_payload;
	memcpy(&size_payload, buffer->data + 1, 2);
	buffer->size = recv(client->socket->desc, buffer->data, size_payload + 3,
			MSG_WAITALL);
	if (!sockets_isBlocked(client->socket)) {
		fcntl(client->socket->desc, F_SETFL, O_NONBLOCK);
	}

	if (buffer->size == -1)
		sockets_setState(client, SOCKETSTATE_DISCONNECTED);

	return buffer->size;
}

void sockets_destroyClient(t_socket_client *client) {
	if (!sockets_isConnected(client))
		client->socket->desc = -1;
	sockets_destroy(client->socket);

	free(client);
}

/*
 *						Server Socket Functions
 */

t_socket_server *sockets_createServer(char *ip, int port) {
	t_socket_server *sckt = malloc(sizeof(t_socket_server));
	if ((sckt->socket = sockets_create(ip, port)) == NULL ) {
		free(sckt);
		return NULL ;
	}
	sckt->maxconexions = DEFAULT_MAX_CONEXIONS;
	return sckt;
}

t_socket *sockets_getServerSocket(t_socket_server* server) {
	return server->socket;
}

void sockets_setMaxConexions(t_socket_server* server, int conexions) {
	server->maxconexions = conexions;
}

int sockets_getMaxConexions(t_socket_server* server) {
	return server->maxconexions;
}

int sockets_listen(t_socket_server* server) {
	if (listen(server->socket->desc, server->maxconexions) == -1) {
		return 0;
	}
	return 1;
}

t_socket_client *sockets_accept(t_socket_server* server) {
	t_socket_client* client = malloc(sizeof(t_socket_client));
	int addrlen = sizeof(struct sockaddr_in);
	client->socket = malloc(sizeof(t_socket));
	client->socket->my_addr = malloc(sizeof(struct sockaddr));

	if (!sockets_isBlocked(server->socket)) {
		fcntl(server->socket->desc, F_SETFL, O_NONBLOCK);
	}

	if ((client->socket->desc = accept(server->socket->desc,
			(struct sockaddr *) client->socket->my_addr, (void *) &addrlen))
			== -1) {
		free(client->socket->my_addr);
		free(client->socket);
		free(client);
		return NULL ;
	}

	if (!sockets_isBlocked(server->socket)) {
		fcntl(server->socket->desc, F_SETFL, O_NONBLOCK);
	}

	sockets_setState(client, SOCKETSTATE_CONNECTED);
	sockets_setMode(client->socket, SOCKETMODE_BLOCK);
	return client;
}

void sockets_select(t_list* servers, t_list* clients, int usec_timeout,
		t_socket_client *(*onAcceptClosure)(t_socket_server*),
		int (*onRecvClosure)(t_socket_client*)) {
	fd_set conexions_set;
	int max_desc = 3;
	int select_return;
	struct timeval tv;
	t_list *close_clients = NULL;
	FD_ZERO(&conexions_set);
	tv.tv_sec = 0;
	tv.tv_usec = usec_timeout;

	void closure_clientsBuildSet(t_socket_client *client) {
		int curr_desc = client->socket->desc;
		FD_SET(curr_desc, &conexions_set);
		if (max_desc < curr_desc)
			max_desc = curr_desc;
	}

	if (clients != NULL && list_size(clients) > 0) {
		list_iterate(clients, (void*) &closure_clientsBuildSet);
	}

	void closure_serversBuildSet(t_socket_server *server) {
		int curr_desc = server->socket->desc;
		FD_SET(curr_desc, &conexions_set);
		if (max_desc < curr_desc)
			max_desc = curr_desc;

	}

	if (servers != NULL && list_size(servers) > 0) {
		list_iterate(servers, (void*) &closure_serversBuildSet);
	}

	if (usec_timeout == 0) {
		select_return = select(max_desc + 1, &conexions_set, NULL, NULL, NULL );
	} else {
		select_return = select(max_desc + 1, &conexions_set, NULL, NULL, &tv);
	}

	if (select_return == -1)
		return;

	void closure_acceptFromSocket(t_socket_server *server) {
		if (FD_ISSET(server->socket->desc, &conexions_set)) {
			t_socket_client *auxClient = NULL;
			if (onAcceptClosure != NULL ) {
				auxClient = onAcceptClosure(server);
			} else if (clients != NULL ) {
				auxClient = sockets_accept(server);
			}

			if (clients != NULL && auxClient != NULL ) {
				list_add(clients, auxClient);
			}
		}
	}
	if (servers != NULL && list_size(servers) > 0) {
		list_iterate(servers, (void*) &closure_acceptFromSocket);
	}

	void closure_recvFromSocket(t_socket_client *client) {
		if (FD_ISSET(client->socket->desc, &conexions_set)) {
			if (onRecvClosure(client) == 0) {
				if (close_clients == NULL )
					close_clients = list_create();
				list_add(close_clients, client);
			}
		}
	}

	if (clients != NULL && list_size(clients) > 0) {
		list_iterate(clients, (void*) &closure_recvFromSocket);
		if (close_clients != NULL ) {
			int index;
			for (index = 0; index < list_size(close_clients); index++) {
				t_socket_client *client = list_get(close_clients, index);
				bool closure_removeSocket(t_socket_client *aux) {
					return client->socket->desc == aux->socket->desc;
				}
				list_remove_by_condition(clients,
						(void*) &closure_removeSocket);
			}
			list_destroy_and_destroy_elements(close_clients,
					(void*) sockets_destroyClient);
		}
	}
}

void sockets_destroyServer(t_socket_server* server) {
	sockets_destroy(server->socket);
	free(server);
}

/**
 * Crea un server y lo pone a escuchar conexiones entrantes.
 */bool sockets_create_little_server(char* ip, int puerto, t_log* logger,
		pthread_mutex_t* log_mutex, char* log_name, t_list *servers,
		t_list* clients, t_socket_client *(*onAcceptClosure)(t_socket_server*),
		int (*onRecvClosure)(t_socket_client*)) {

	t_socket_server* server = sockets_createServer(ip, puerto);

	if (!sockets_listen(server)) {
		if (log_mutex != NULL ) {
			pthread_mutex_lock(log_mutex);
		}
		log_error(logger, "%s: No se puede escuchar", log_name);

		if (log_mutex != NULL ) {
			pthread_mutex_unlock(log_mutex);
		}

		sockets_destroyServer(server);
		return false;
	}

	if (log_mutex != NULL ) {
		pthread_mutex_lock(log_mutex);
	}

	log_info(logger, "%s: Escuchando conexiones entrantes en %s:%d", log_name,
			ip, puerto);

	if (log_mutex != NULL ) {
		pthread_mutex_unlock(log_mutex);
	}

	list_add(servers, server);

	while (true) {
		if (log_mutex != NULL ) {
			pthread_mutex_lock(log_mutex);
		}
		log_debug(logger, "%s: Entro al select", log_name);

		if (log_mutex != NULL ) {
			pthread_mutex_unlock(log_mutex);
		}

		sockets_select(servers, clients, 0, onAcceptClosure, onRecvClosure);
	}

	return true;

}

 t_socket_client* sockets_conectar_a_servidor(char* mi_ip,
		int mi_puerto, char* server_ip, int server_puerto, t_log* logger,
		int handshake_type, char* handshake_msg, char* handshake_success,
		char* server_name) {
	t_socket_client* socket_client = sockets_createClient(mi_ip, mi_puerto);

	if (socket_client == NULL ) {
		log_error(logger, "Error al crear el socket");
		return NULL;
	}

	if (sockets_connect(socket_client, server_ip, server_puerto) == 0) {
		log_error(logger, "Error al conectar con %s", server_name);
		return NULL;
	}

	log_info(logger, "Conectando con %s...", server_name);
	log_debug(logger, "Enviando handshake");

	t_mensaje* mensaje = mensaje_create(handshake_type);
	mensaje_setdata(mensaje, string_duplicate(handshake_msg),
			strlen(handshake_msg) + 1);
	mensaje_send(mensaje, socket_client);
	mensaje_destroy(mensaje);

	t_socket_buffer* buffer = sockets_recv(socket_client);

	if (buffer == NULL ) {
		log_error(logger, "Error al recibir la respuesta del handshake");
		return NULL;
	}

	t_mensaje* rta_handshake = mensaje_deserializer(buffer, 0);
	sockets_bufferDestroy(buffer);

	if (rta_handshake->length != (strlen(handshake_success) + 1)
			|| (!string_equals_ignore_case((char*) rta_handshake->payload,
					handshake_success))) {
		log_error(logger, "Error en la respuesta del handshake");
		mensaje_destroy(rta_handshake);
		return NULL;
	}

	mensaje_destroy(rta_handshake);

	log_info(logger, "Conectado con %s: Origen: 127.0.0.1:%d, Destino: %s:%d",
			server_name, mi_puerto, server_ip, server_puerto);
	return socket_client;
}
