/*
 * sockets.h
 *
 *  Created on: 12/05/2013
 *      Author: reyiyo
 */

#ifndef OLD_SOCKETS_H_
#define OLD_SOCKETS_H_

#define _XOPEN_SOURCE 500

#include <sys/select.h>
#include <netinet/in.h>
#include <commons/collections/list.h>

#define DEFAULT_BUFFER_SIZE 	1500
#define DEFAULT_MAX_CONEXIONS 	100
#define SELECT_USEC_TIMEOUT 	500

typedef enum {
	SOCKETSTATE_CONNECTED, SOCKETSTATE_DISCONNECTED
} e_socket_state;

typedef enum {
	SOCKETMODE_NONBLOCK = 1, SOCKETMODE_BLOCK = 2
} e_socket_mode;

typedef struct {
	int desc;
	struct sockaddr *my_addr;
	e_socket_mode mode;
} t_socket;

typedef struct {
	t_socket* socket;
	t_socket* serv_socket;
	e_socket_state state;
} t_socket_client;

typedef struct {
	t_socket *socket;
	int maxconexions;
} t_socket_server;

typedef struct {
	char data[DEFAULT_BUFFER_SIZE];
	int size;
} t_socket_buffer;

typedef struct {
	void *serializated_data;
	int size;
} t_socket_sbuffer;

void sockets_bufferDestroy(t_socket_buffer *tbuffer);
void sockets_sbufferDestroy(t_socket_sbuffer *tbuffer);

t_socket *sockets_getClientSocket(t_socket_client *client);
t_socket *sockets_getServerSocket(t_socket_server *server);

char *sockets_getIp(t_socket *sckt);
void sockets_getIpAsBytes(t_socket *sckt, unsigned char ip[]);
void sockets_getStringIpAsBytes(char *str_ip, unsigned char ip[]);
int sockets_getPort(t_socket *sckt);

void sockets_setMode(t_socket *sckt, e_socket_mode mode);
e_socket_mode sockets_getMode(t_socket *sckt);
int sockets_isBlocked(t_socket *sckt);

t_socket_client *sockets_createClient(char *ip, int port);
t_socket_client *sockets_createClientUnix(char *path);
int sockets_isConnected(t_socket_client *client);
int sockets_equalsClients(t_socket_client *client1, t_socket_client *client2);
e_socket_state sockets_getState(t_socket_client *client);
void sockets_setState(t_socket_client *client, e_socket_state state);
int sockets_connect(t_socket_client *client, char *server_ip, int server_port);
int sockets_connectUnix(t_socket_client *client, char *path);
int sockets_send(t_socket_client *client, void *data, int datalen);
int sockets_write(t_socket_client *client, void *data, int datalen);
int sockets_sendBuffer(t_socket_client *client, t_socket_buffer *buffer);
int sockets_bindUnix(t_socket *sckt, char *path);
int sockets_sendSBuffer(t_socket_client *client, t_socket_sbuffer *buffer);
int sockets_sendString(t_socket_client *client, char *str);
int sockets_sendSerialized(t_socket_client *client, void *data,
		t_socket_sbuffer *(*serializer)(void*));
t_socket_buffer *sockets_recv(t_socket_client *client);
int sockets_recvInBuffer(t_socket_client *client, t_socket_buffer *buffer);
int sockets_recvInSBuffer(t_socket_client *client, t_socket_sbuffer *buffer);
void *sockets_recvSerialized(t_socket_client *client,
		void *(*deserializer)(t_socket_sbuffer*));
void sockets_destroyClient(t_socket_client *client);

t_socket_server *sockets_createServer(char *ip, int port);
t_socket_server *sockets_createServerUnix(char *path);
t_socket *sockets_createUnix(char *path);
void sockets_setMaxConexions(t_socket_server* server, int conexions);
int sockets_getMaxConexions(t_socket_server* server);
int sockets_listen(t_socket_server* server);
t_socket_client *sockets_accept(t_socket_server* server);
t_socket_client *sockets_acceptUnix(t_socket_server* server);
void sockets_select(t_list* servers, t_list *clients, int usec_timeout,
		t_socket_client *(*onAcceptClosure)(t_socket_server*),
		int (*onRecvClosure)(t_socket_client*));
void sockets_destroyServer(t_socket_server* server);

#endif /* OLD_SOCKETS_H_ */
