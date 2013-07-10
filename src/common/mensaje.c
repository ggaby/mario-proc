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

void stream_destroy(t_stream* self) {
	free(self->data);
	free(self);
}

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

t_stream* get_info_nivel_response_create_serialized(t_connection_info* nivel,
		t_connection_info* planificador) {

	t_stream* niv = connection_info_serialize(nivel);
	t_stream* planif = connection_info_serialize(planificador);
	t_stream* result = malloc(sizeof(t_stream));
	result->data = malloc(niv->length + planif->length);
	memcpy(result->data, niv->data, niv->length);
	memcpy(result->data + niv->length, planif->data, planif->length);
	result->length = niv->length + planif->length;
	stream_destroy(niv);
	stream_destroy(planif);
	return result;
}

//Sí, aguante el código spaguetti (?)

t_get_info_nivel_response* get_info_nivel_response_deserialize(char* data) {
	t_get_info_nivel_response* new = malloc(sizeof(t_get_info_nivel_response));

	t_connection_info* nivel = malloc(sizeof(t_connection_info));
	t_connection_info* planificador = malloc(sizeof(t_connection_info));
	int tmpsize, offset = 0;

	for (tmpsize = 1; (data + offset)[tmpsize - 1] != '\0'; tmpsize++)
		;
	nivel->ip = malloc(tmpsize);
	memcpy(nivel->ip, data + offset, tmpsize);
	offset += tmpsize;
	memcpy(&nivel->puerto, data + offset, tmpsize = sizeof(uint32_t));
	offset += tmpsize;

	new->nivel = nivel;

	for (tmpsize = 1; (data + offset)[tmpsize - 1] != '\0'; tmpsize++)
		;
	planificador->ip = malloc(tmpsize);
	memcpy(planificador->ip, data + offset, tmpsize);
	offset += tmpsize;
	memcpy(&planificador->puerto, data + offset, sizeof(uint32_t));

	new->planificador = planificador;
	return new;
}

void get_info_nivel_response_destroy(t_get_info_nivel_response* self) {
	connection_destroy(self->nivel);
	connection_destroy(self->planificador);
	free(self);
}

t_stream* connection_info_serialize(t_connection_info* self) {
	t_stream* result = malloc(sizeof(t_stream));
	char* buffer = malloc(sizeof(uint32_t) + strlen(self->ip) + 1);
	int tmpsize, offset = 0;

	memcpy(buffer, self->ip, tmpsize = strlen(self->ip) + 1);
	offset = tmpsize;

	memcpy(buffer + offset, &self->puerto, tmpsize = sizeof(uint32_t));
	offset += tmpsize;

	result->data = buffer;
	result->length = offset;
	return result;
}

t_connection_info* connection_info_deserialize(char* data) {
	t_connection_info* new = malloc(sizeof(t_connection_info));
	int tmpsize, offset = 0;

	for (tmpsize = 1; (data + offset)[tmpsize - 1] != '\0'; tmpsize++)
		;
	new->ip = malloc(tmpsize);
	memcpy(new->ip, data + offset, tmpsize);
	offset += tmpsize;

	memcpy(&new->puerto, data + offset, sizeof(uint32_t));

	return new;
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

t_connection_info* connection_create(char* ip_y_puerto) {
	t_connection_info* new = malloc(sizeof(t_connection_info));
	char** tokens = string_split(ip_y_puerto, ":");
	new->ip = string_duplicate(tokens[0]);
	new->puerto = atoi(tokens[1]);
	array_destroy(tokens);
	return new;
}

char* connection_info_to_string(t_connection_info* connection) {
	return string_from_format("%s:%d", connection->ip, connection->puerto);
}

void connection_destroy(t_connection_info* self) {
	free(self->ip);
	free(self);
}

void responder_handshake(t_socket_client* client, t_log* logger,
		pthread_mutex_t* log_mutex, char* name) {
	if (log_mutex != NULL ) {
		pthread_mutex_lock(log_mutex);
	}
	log_info(logger, "%s: Cliente conectado por el socket %d", name,
			client->socket->desc);
	if (log_mutex != NULL ) {
		pthread_mutex_unlock(log_mutex);
	}
	t_mensaje* mensaje = mensaje_create(M_HANDSHAKE_RESPONSE);
	mensaje_setdata(mensaje, strdup(HANDSHAKE_SUCCESS),
			strlen(HANDSHAKE_SUCCESS) + 1);
	mensaje_send(mensaje, client);
	mensaje_destroy(mensaje);
}
