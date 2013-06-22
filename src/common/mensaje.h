/*
 * mensaje.h
 *
 *  Created on: 02/06/2013
 *      Author: reyiyo
 */

#ifndef MENSAJE_H_
#define MENSAJE_H_

#include <stdint.h>
#include <stdlib.h>
#include "sockets.h"
#include <string.h>

#define PERSONAJE_HANDSHAKE "Aquí un personaje"
#define M_HANDSHAKE 1
#define M_ERROR 2
#define M_GET_INFO_NIVEL_REQUEST 3
#define M_GET_INFO_NIVEL_RESPONSE 4

typedef struct t_stream {
	int length;
	char* data;
} t_stream;

typedef struct t_mensaje {
	uint8_t type;
	uint16_t length;
	void* payload;
}__attribute__ ((packed)) t_mensaje;

typedef struct t_connection_info {
	char* ip;
	uint32_t puerto;
} __attribute__ ((packed)) t_connection_info;

t_mensaje *mensaje_create(uint8_t type);
void mensaje_setdata(t_mensaje* mensaje, void* data, uint16_t length);
void* mensaje_getdata(t_mensaje* mensaje);
void mensaje_destroy(t_mensaje* mensaje);
void* mensaje_getdata_destroy(t_mensaje* mensaje);
t_socket_buffer* mensaje_serializer(t_mensaje* mensaje);
t_mensaje* mensaje_deserializer(t_socket_buffer* buffer, uint32_t dataStart);
t_mensaje* mensaje_clone(t_mensaje* mensaje);
void mensaje_send(t_mensaje* mensaje, t_socket_client *client);
t_connection_info* t_connection_new(char* ip_y_puerto);
void t_connection_destroy(t_connection_info* self);
//t_stream* t_connection_info_serialize(t_connection_info* self);
//t_connection_info* t_connection_info_deserialize(char* data);
char* t_connection_info_to_string(t_connection_info* connection);
#endif /* MENSAJE_H_ */
