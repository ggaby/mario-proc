/*
 * Orquestador.c
 *
 *  Created on: 20/06/2013
 *      Author: reyiyo
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <commons/string.h>
#include "Orquestador.h"
#include "../../common/list.h"
#include "../Planificador/Planificador.h"
#include "../../common/common_structs.h"
#include <unistd.h>
#include <commons/config.h>

#define PUERTO_ORQUESTADOR 5000

void* orquestador(void* plat) {
	t_plataforma* plataforma = (t_plataforma*) plat;
	t_orquestador* self = orquestador_create(PUERTO_ORQUESTADOR, plataforma);

	t_socket_client* acceptClosure(t_socket_server* server) {
		t_socket_client* client = sockets_accept(server);

		t_mensaje* mensaje = mensaje_recibir(client);

		if (mensaje == NULL) {
			sockets_destroyClient(client);
			pthread_mutex_lock(&plataforma->logger_mutex);
			log_warning(plataforma->logger,
					"%s:%d -> Orquestador - Error al recibir datos en el accept",
					sockets_getIp(client->socket),
					sockets_getPort(client->socket));
			pthread_mutex_unlock(&plataforma->logger_mutex);
			return NULL;
		}

		int tipo_mensaje = mensaje->type;

		mensaje_destroy(mensaje);

		switch (tipo_mensaje) {
		case M_HANDSHAKE_PERSONAJE:
			responder_handshake(client, plataforma->logger,
					&plataforma->logger_mutex, "Orquestador");
			break;
		case M_HANDSHAKE_NIVEL:
			responder_handshake(client, plataforma->logger,
					&plataforma->logger_mutex, "Orquestador");
			if (!procesar_handshake_nivel(self, client, plataforma)) {
				orquestador_send_error_message("Error al procesar el handshake",
						client);
				return NULL;
			}
			break;
		default:
			pthread_mutex_lock(&plataforma->logger_mutex);
			char* error_msg = string_from_format(
					"Tipo del mensaje recibido no válido tipo: %d",
					tipo_mensaje);
			log_warning(plataforma->logger, error_msg);
			mensaje_create_and_send(M_ERROR, string_duplicate(error_msg),
					strlen(error_msg) + 1, client);
			free(error_msg);
			pthread_mutex_unlock(&plataforma->logger_mutex);
			orquestador_send_error_message("Request desconocido", client);
			return NULL;
		}

		return client;
	}

	int recvClosure(t_socket_client* client) {

		t_mensaje* mensaje = mensaje_recibir(client);

		if (mensaje == NULL) {
			pthread_mutex_lock(&plataforma->logger_mutex);
			log_debug(plataforma->logger,
					"%s:%d -> Nivel - Mensaje recibido NULL",
					sockets_getIp(client->socket),
					sockets_getPort(client->socket));
			pthread_mutex_unlock(&plataforma->logger_mutex);
			verificar_nivel_desconectado(plataforma, client);
			return false;
		}

		process_request(mensaje, client, plataforma);

		mensaje_destroy(mensaje);
		return true;
	}

	sockets_create_little_server(plataforma->ip, self->puerto,
			plataforma->logger, &plataforma->logger_mutex, "Orquestador",
			self->servers, self->clients, &acceptClosure, &recvClosure, NULL);

	orquestador_destroy(self);

	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger, "Orquestador: Server cerrado correctamente");
	pthread_mutex_unlock(&plataforma->logger_mutex);

	return (void*) EXIT_SUCCESS;

}

t_orquestador* orquestador_create(int puerto, t_plataforma* plataforma) {
	t_orquestador* new = malloc(sizeof(t_orquestador));
	new->puerto = puerto;
	new->planificadores_count = 0;
	new->clients = list_create();
	new->servers = list_create();
	plataforma->orquestador = new;
	return new;
}

void orquestador_destroy(t_orquestador* self) {
	list_destroy_and_destroy_elements(self->clients,
			(void *) sockets_destroyClient);
	list_destroy_and_destroy_elements(self->servers,
			(void *) sockets_destroyServer);
	free(self);
}

void process_request(t_mensaje* request, t_socket_client* client,
		t_plataforma* plataforma) {

	switch (request->type) {
	case M_GET_INFO_NIVEL_REQUEST:
		orquestador_get_info_nivel(request, client, plataforma);
		break;

	case M_FIN_DE_NIVEL:
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_info(plataforma->logger,
				"Orquestador: El personaje en el socket %d ha finalizado el nivel",
				client->socket->desc);
		pthread_mutex_unlock(&plataforma->logger_mutex);

		if (!hay_personajes_jugando(plataforma)) {
			pthread_mutex_lock(&plataforma->logger_mutex);
			log_info(plataforma->logger,
					"Orquestador: Todos los personajes terminaron los niveles.");
			pthread_mutex_unlock(&plataforma->logger_mutex);

			ejecutar_koopa(plataforma);
		}

		break;
	case M_RECURSOS_LIBERADOS:
		orquestador_liberar_recursos(plataforma, client,
				string_duplicate((char*) request->payload));
		break;
	case M_DEADLOCK_DETECTADO:
		orquestador_handler_deadlock(string_duplicate(request->payload),
				plataforma, client);
		break;

	default:
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_warning(plataforma->logger,
				"Orquestador: Tipo de Request desconocido: %d", request->type);
		pthread_mutex_unlock(&plataforma->logger_mutex);
		orquestador_send_error_message("Request desconocido", client);
	}
}

void orquestador_get_info_nivel(t_mensaje* request, t_socket_client* client,
		t_plataforma* plataforma) {
	char* nivel_pedido = string_duplicate((char*) request->payload);

	plataforma_t_nivel* el_nivel = plataforma_get_nivel_by_nombre(plataforma,
			nivel_pedido);

	if (el_nivel == NULL) {
		char* error_msg = string_from_format("Nivel inválido: %s",
				nivel_pedido);
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_error(plataforma->logger, "Orquestador -> Personaje: %s",
				error_msg);
		pthread_mutex_unlock(&plataforma->logger_mutex);

		free(nivel_pedido);
		orquestador_send_error_message(error_msg, client);
		free(error_msg);
		return;
	}

	t_mensaje* response = mensaje_create(M_GET_INFO_NIVEL_RESPONSE);

	t_stream* response_data = get_info_nivel_response_create_serialized(
			el_nivel->connection_info, el_nivel->planificador->connection_info);

	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger,
			"Orquestador -> Personaje: Enviando información de nivel %s",
			nivel_pedido);
	pthread_mutex_unlock(&plataforma->logger_mutex);
	mensaje_setdata(response, response_data->data, response_data->length);
	mensaje_send(response, client);
	mensaje_destroy(response);

	free(nivel_pedido);
	free(response_data);
}

void orquestador_send_error_message(char* error_description,
		t_socket_client* client) {

	mensaje_create_and_send(M_ERROR, strdup(error_description),
			strlen(error_description) + 1, client);

}

bool procesar_handshake_nivel(t_orquestador* self,
		t_socket_client* socket_nivel, t_plataforma* plataforma) {

	t_mensaje* mensaje = mensaje_create(M_GET_NOMBRE_NIVEL_REQUEST);
	mensaje_send(mensaje, socket_nivel);
	mensaje_destroy(mensaje);

	mensaje = mensaje_recibir(socket_nivel);

	if (mensaje == NULL) {
		sockets_destroyClient(socket_nivel);
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_warning(plataforma->logger,
				"Orquestador -> Nivel - Handshake recibido inválido");
		pthread_mutex_unlock(&plataforma->logger_mutex);
		return false;
	}

	if (mensaje->type != M_GET_NOMBRE_NIVEL_RESPONSE) {
		sockets_destroyClient(socket_nivel);
		mensaje_destroy(mensaje);
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_warning(plataforma->logger,
				"Orquestador -> Nivel - Handshake recibido inválido");
		pthread_mutex_unlock(&plataforma->logger_mutex);
		return false;
	}

	char* planificador_connection_info = string_from_format("%s:%d",
			plataforma->ip, 9000 + self->planificadores_count);

	if (plataforma_create_nivel(plataforma, mensaje->payload, socket_nivel,
			planificador_connection_info) != 0) {
		mensaje_destroy(mensaje);
		sockets_destroyClient(socket_nivel);

		pthread_mutex_lock(&plataforma->logger_mutex);
		log_error(plataforma->logger,
				"Orquestador: No se pudo crear el planificador para el nivel %s",
				mensaje->payload);
		pthread_mutex_unlock(&plataforma->logger_mutex);

		return false;
	}

	self->planificadores_count++;

	free(planificador_connection_info);
	mensaje_destroy(mensaje);
	return true;
}

void verificar_nivel_desconectado(t_plataforma* plataforma,
		t_socket_client* client) {

	bool es_el_nivel(plataforma_t_nivel* elem) {
		return sockets_equalsClients(client, elem->socket_nivel);
	}

	plataforma_t_nivel* nivel_desconectado = list_find(plataforma->niveles,
			(void*) es_el_nivel);

	if (nivel_desconectado != NULL) {
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_info(plataforma->logger,
				"Orquestador: El nivel %s se ha desconectado",
				nivel_desconectado->nombre);
		pthread_mutex_unlock(&plataforma->logger_mutex);

		planificador_destroy(nivel_desconectado->planificador);

		pthread_cancel(nivel_desconectado->thread_planificador);

		my_list_remove_and_destroy_by_condition(plataforma->niveles,
				(void*) es_el_nivel, (void*) plataforma_nivel_destroy);

		pthread_mutex_lock(&plataforma->logger_mutex);
		log_debug(plataforma->logger,
				"Orquestador: Se terminó de limpiar las estructuras");
		pthread_mutex_unlock(&plataforma->logger_mutex);
	}

}

void orquestador_handler_deadlock(char* ids_personajes_en_deadlock,
		t_plataforma* plataforma, t_socket_client* socket_nivel) {

	planificador_t_personaje* victima = orquestador_seleccionar_victima(
			ids_personajes_en_deadlock, plataforma, socket_nivel);

	orquestador_informar_victima_al_nivel(plataforma, victima, socket_nivel);
	orquestador_matar_personaje(plataforma, victima);

}

planificador_t_personaje* orquestador_seleccionar_victima(
		char* ids_personajes_en_deadlock, t_plataforma* plataforma,
		t_socket_client* socket_nivel) {

	plataforma_t_nivel* nivel = plataforma_get_nivel_by_socket(plataforma,
			socket_nivel);

	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger,
			"Orquestador: Deadlock detectado en nivel %s entre: %s",
			nivel->nombre, ids_personajes_en_deadlock);
	pthread_mutex_unlock(&plataforma->logger_mutex);

	t_list* personajes = list_create();

	char** ids = string_split(ids_personajes_en_deadlock, " ");

	void buscar_y_agregar_en_lista(char* id) {
		planificador_t_personaje* el_personaje =
				buscar_personaje_bloqueado_by_id(nivel->planificador, id[0]);

		if (el_personaje != NULL) {
			list_add(personajes, el_personaje);
		}
	}

	string_iterate_lines(ids, (void*) buscar_y_agregar_en_lista);
	array_destroy(ids);

	int temporal_compare_to(char* t1, char* t2) {
		char** time1 = string_split(t1, ":");
		char** time2 = string_split(t2, ":");

		int index = 0;

		while (time1[index] != NULL && time2[index] != NULL) {
			int v1 = atoi(time1[index]);
			int v2 = atoi(time2[index]);

			if (v1 < v2) {
				array_destroy(time1);
				array_destroy(time2);
				return -1;
			}

			if (v1 > v2) {
				array_destroy(time1);
				array_destroy(time2);
				return 1;
			}

			index++;
		}

		array_destroy(time1);
		array_destroy(time2);

		return 0;
	}

	bool comparator(planificador_t_personaje* p1, planificador_t_personaje* p2) {
		return temporal_compare_to(p1->tiempo_llegada, p2->tiempo_llegada) < 0;
	}

	list_sort(personajes, (void*) comparator);

	planificador_t_personaje* oscar = list_get(personajes, 0);

	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger, "Orquestador: victima: %c", oscar->id);
	pthread_mutex_unlock(&plataforma->logger_mutex);

	return oscar;

}

void orquestador_liberar_recursos(t_plataforma* plataforma,
		t_socket_client* client, char* recursos_str) {

	t_list* recursos_liberados = parsear_recursos(recursos_str);
	char* recursos_asignados = string_new();

	bool es_el_nivel(plataforma_t_nivel* elem) {
		return sockets_equalsClients(client, elem->socket_nivel);
	}

	plataforma_t_nivel* nivel = list_find(plataforma->niveles,
			(void*) es_el_nivel);

	void liberar_recurso(t_recurso* elem) {
		int i;
		for (i = 0; i < elem->cantidad; i++) {
			planificador_t_personaje* personaje_desbloqueado =
					planificador_recurso_liberado(plataforma,
							nivel->planificador, elem->simbolo);
			if (personaje_desbloqueado != NULL) {
				char* asignacion = string_from_format("%c%c,",
						personaje_desbloqueado->id, elem->simbolo);
				string_append(&recursos_asignados,
						string_duplicate(asignacion));
				free(asignacion);
			}
		}
	}

	pthread_mutex_lock(&nivel->planificador->planificador_mutex);
	list_iterate(recursos_liberados, (void*) liberar_recurso);
	pthread_mutex_unlock(&nivel->planificador->planificador_mutex);

	list_destroy_and_destroy_elements(recursos_liberados,
			(void*) recurso_destroy);

	mensaje_create_and_send(M_RECURSOS_ASIGNADOS,
			string_duplicate(recursos_asignados),
			strlen(recursos_asignados) + 1, client);
	free(recursos_asignados);
	free(recursos_str);
}

t_list* parsear_recursos(char* recursos_str) {
	t_list* recursos = list_create();
	char* recursos_str2 = string_substring(recursos_str, 0,
			strlen(recursos_str) - 1);
	char** recursos_arr = string_split(recursos_str2, ",");
	int index = 0;
	while (recursos_arr[index] != NULL) {
		char* recurso = recursos_arr[index];
		list_add(recursos,
				recurso_create(NULL, recurso[0], atoi(recurso + 1), NULL));
		index++;
	}

	array_destroy(recursos_arr);
	free(recursos_str2);
	return recursos;
}

void orquestador_matar_personaje(t_plataforma* plataforma,
		planificador_t_personaje* victima) {
	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger, "Orquestador: Matando al personaje %c",
			victima->id);
	pthread_mutex_unlock(&plataforma->logger_mutex);
	mensaje_create_and_send(M_MORITE_GIL, NULL, 0, victima->socket);
}

void orquestador_informar_victima_al_nivel(t_plataforma* plataforma,
		planificador_t_personaje* victima, t_socket_client* socket_nivel) {
	char* victima_str = string_from_format("%c", victima->id);
	mensaje_create_and_send(M_VICTIMA_SELECCIONADA,
			string_duplicate(victima_str), strlen(victima_str) + 1,
			socket_nivel);
	free(victima_str);
}

bool hay_personajes_jugando(t_plataforma* plataforma) {

	bool hay_personajes = false;

	void hay_personajes_en_nivel(plataforma_t_nivel* nivel) {
		if (!hay_personajes) {
			if (nivel->planificador->personaje_ejecutando != NULL) {
				hay_personajes = true;
			}

			if (!queue_is_empty(nivel->planificador->personajes_ready)) {
				hay_personajes = true;
			}

			void personajes_bloqueados(char* key, t_queue* queue) {
				if (!queue_is_empty(queue)) {
					hay_personajes = true;
				}
			}

			dictionary_iterator(nivel->planificador->personajes_blocked,
					(void*) personajes_bloqueados);
		}
	}

	list_iterate(plataforma->niveles, (void*) hay_personajes_en_nivel);

	return hay_personajes;
}

void ejecutar_koopa(t_plataforma* plataforma) {
	pthread_mutex_lock(&plataforma->logger_mutex);
	log_info(plataforma->logger,
			"Orquestador: Juntando fuerzas para enfrentar a koopa.");
	pthread_mutex_unlock(&plataforma->logger_mutex);

	t_config* config = config_create(plataforma->config_path);
	if (!config_has_property(config, "dondeEstaKoopa")
			|| !config_has_property(config, "koopaParamPath")) {
		pthread_mutex_lock(&plataforma->logger_mutex);
		log_warning(plataforma->logger,
				"El path de koopa o de sus parametros no se encuentra en la configuracion.");
		pthread_mutex_unlock(&plataforma->logger_mutex);
		config_destroy(config);
		return;
	}

	char* donde_esta_koopa = strdup(
			config_get_string_value(config, "dondeEstaKoopa"));

	char* koopa_param = strdup(
			config_get_string_value(config, "koopaParamPath"));

	config_destroy(config);

	plataforma_destroy(plataforma);
	execl(donde_esta_koopa, "koopa", koopa_param, NULL);
	free(donde_esta_koopa);
	free(koopa_param);
}
