/*
 * mensaje.c
 *
 *  Created on: 02/06/2013
 *      Author: reyiyo
 */

#include "mensaje.h"
#include <assert.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>

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
