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
#include <stdbool.h>
#include "sockets.h"
#include <string.h>

//Tipos de Mensajes
//Handshakes (1-10)
#define M_HANDSHAKE_PERSONAJE 1
#define M_HANDSHAKE_NIVEL 2
//Errores (11-20)
#define M_ERROR 11

//TODO No puede separarse en request y response ya que depende de donde se vea... o no?
//Request(21-40)
#define M_GET_INFO_NIVEL_REQUEST 21
#define M_GET_NOMBRE_NIVEL_REQUEST 22
//Response(41-60)
#define M_GET_INFO_NIVEL_RESPONSE 41
#define M_HANDSHAKE_RESPONSE 42
#define M_GET_NOMBRE_NIVEL_RESPONSE 43

#define M_TURNO_FINALIZADO 44
#define M_NOTIFICACION_MOVIMIENTO 45

//Contenidos de Mensajes
#define PERSONAJE_HANDSHAKE "Aquí un personaje"
#define NIVEL_HANDSHAKE "Aqui un nivel"
#define HANDSHAKE_SUCCESS "OK"

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

typedef struct t_get_info_nivel_response {
	t_connection_info* nivel;
	t_connection_info* planificador;
} __attribute__ ((packed)) t_get_info_nivel_response;

void stream_destroy(t_stream* self);
t_mensaje *mensaje_create(uint8_t type);
void mensaje_setdata(t_mensaje* mensaje, void* data, uint16_t length);
void* mensaje_getdata(t_mensaje* mensaje);
void mensaje_destroy(t_mensaje* mensaje);
void* mensaje_getdata_destroy(t_mensaje* mensaje);
t_socket_buffer* mensaje_serializer(t_mensaje* mensaje);
t_mensaje* mensaje_deserializer(t_socket_buffer* buffer, uint32_t dataStart);
t_mensaje* mensaje_clone(t_mensaje* mensaje);
void mensaje_send(t_mensaje* mensaje, t_socket_client *client);
t_connection_info* connection_create(char* ip_y_puerto);
void connection_destroy(t_connection_info* self);
t_stream* connection_info_serialize(t_connection_info* self);
t_connection_info* connection_info_deserialize(char* data);
char* connection_info_to_string(t_connection_info* connection);
t_stream* get_info_nivel_response_create_serialized(t_connection_info* nivel,
		t_connection_info* planificador);
t_get_info_nivel_response* get_info_nivel_response_deserialize(char* data);
void get_info_nivel_response_destroy(t_get_info_nivel_response* self);
void responder_handshake(t_socket_client* client, t_log* logger,
		pthread_mutex_t* log_mutex, char* name);
#endif /* MENSAJE_H_ */
