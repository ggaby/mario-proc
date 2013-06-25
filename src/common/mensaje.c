/*
 * mensaje.c
 *
 *  Created on: 02/06/2013
 *      Author: reyiyo
 */

#include "common_structs.h"
#include "mensaje.h"
#include <stdio.h>
#include <commons/string.h>

t_mensaje* mensaje_create(uint8_t type) {
	t_mensaje* mensaje = (t_mensaje*) malloc(sizeof(t_mensaje));
	mensaje->type = type;
	mensaje->length = 0;
	mensaje->payload = NULL;

	return mensaje;
}

void mensaje_setdata(t_mensaje* mensaje, void* data, uint16_t length) {
	mensaje->length = length;
	mensaje->payload = data;
}

void* mensaje_getdata(t_mensaje* mensaje) {
	return mensaje->payload;
}

void mensaje_destroy(t_mensaje* mensaje) {
	free(mensaje->payload);
	free(mensaje);
}

//TODO en esta serialización hay algo mal... lo dejo comentado por si
//sive en el futuro


//t_stream* t_connection_info_serialize(t_connection_info* self) {
//	char* data = malloc(sizeof(uint32_t) + strlen(self->ip) + 1);
//	t_stream* stream = malloc(sizeof(t_stream));
//	int tmpsize, offset = 0;
//
//	memcpy(data, &self->ip, tmpsize = strlen(self->ip) + 1);
//	offset = tmpsize;
//
//	memcpy(data + offset, &self->puerto, tmpsize = sizeof(uint32_t));
//
//	stream->length = offset + tmpsize;
//	stream->data = data;
//	printf("%s\n", data);
//
//	return stream;
//}
//
//t_connection_info* t_connection_info_deserialize(char* data) {
//	t_connection_info* self = malloc(sizeof(t_connection_info));
//	int tmpsize, offset = 0;
//
//	for (tmpsize = 1; (data + offset)[tmpsize -1] != '\0'; tmpsize++);
//	self->ip = malloc(tmpsize);
//	memcpy(&self->ip, data + offset, tmpsize);
//	offset += tmpsize;
//
//	memcpy(&self->puerto, data + offset, sizeof(uint32_t));
//
//	return self;
//}

t_socket_buffer* mensaje_serializer(t_mensaje* mensaje) {
	int tmpsize, offset = 0;
	t_socket_buffer* buffer = malloc(sizeof(t_socket_buffer));

	memcpy(buffer->data, &mensaje->type, tmpsize = sizeof(uint8_t));
	offset += tmpsize;

	memcpy(buffer->data + offset, &mensaje->length, tmpsize = sizeof(uint16_t));
	offset += tmpsize;

	memcpy(buffer->data + offset, mensaje->payload, mensaje->length);
	buffer->size = offset + mensaje->length;

	return buffer;
}

t_mensaje* mensaje_deserializer(t_socket_buffer* buffer,
		uint32_t offsetinBuffer) {
	int tmpsize, offset;
	t_mensaje* mensaje = malloc(sizeof(t_mensaje));

	memcpy(&mensaje->type, buffer->data, offset = sizeof(uint8_t));
	memcpy(&mensaje->length, buffer->data + offset, tmpsize = sizeof(uint16_t));
	offset += tmpsize;

	mensaje->payload = malloc(mensaje->length);
	memcpy(mensaje->payload, buffer->data + offset, mensaje->length);

	return mensaje;
}

t_mensaje* mensaje_clone(t_mensaje* mensaje) {
	t_mensaje *new = malloc(sizeof(t_mensaje));
	new->type = mensaje->type;
	new->length = mensaje->length;

	new->payload = malloc(sizeof(mensaje->length));
	memcpy(new->payload, mensaje->payload, mensaje->length);

	return new;
}

void mensaje_send(t_mensaje* mensaje, t_socket_client* client) {
	t_socket_buffer* buffer = mensaje_serializer(mensaje);
	sockets_sendBuffer(client, buffer);
	sockets_bufferDestroy(buffer);
}

bool mensaje_validar_handshake(t_socket_client* client, t_mensaje *rq, char* handshakeEsperado){
	if (rq->type != M_HANDSHAKE) {
			printf("Handshake inválido!\n");
			//log_warning(log, "FSLISTENER", "Handshake invalido");
			return false;
		}
		if (!string_equals_ignore_case((char*) rq->payload, handshakeEsperado)) {
			printf("Handshake inválido!\n");
			//log_warning(log, "FSLISTENER", "Handshake invalido");
			return false;
		}
		printf("Hanshake válido!\n");
		t_mensaje* mensaje = mensaje_create(M_HANDSHAKE);
		mensaje_setdata(mensaje, strdup("OK"), strlen("OK") + 1);
		mensaje_send(mensaje, client);
		mensaje_destroy(mensaje);
		return true;
}

t_connection_info* t_connection_new(char* ip_y_puerto) {
	t_connection_info* new = malloc(sizeof(t_connection_info));
	char** tokens = string_split(ip_y_puerto, ":");
	new->ip = string_duplicate(tokens[0]);
	new->puerto = atoi(tokens[1]);
	array_destroy(tokens);
	return new;
}

char* t_connection_info_to_string(t_connection_info* connection) {
	return string_from_format("%s:%d", connection->ip, connection->puerto);
}
void t_connection_destroy(t_connection_info* self) {
	free(self->ip);
	free(self);
}
