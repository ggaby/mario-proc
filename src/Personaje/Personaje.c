/*
 ============================================================================
 Name        : Personaje.c
 Author      : reyiyo
 Version     :
 Copyright   : GPLv3
 Description : Main del proceso Personaje
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <commons/string.h>
#include "Personaje.h"

bool verificar_argumentos(int argc, char* argv[]);
void personaje_avisar_fin_de_plan_niveles(t_personaje* self);

sig_atomic_t reiniciar_nivel = 0;
sig_atomic_t reiniciar_flan = 0;

int main(int argc, char* argv[]) {

	if (!verificar_argumentos(argc, argv)) {
		return EXIT_FAILURE;
	}

	t_personaje* self = personaje_create(argv[1]);

	if (self == NULL ) {
		return EXIT_FAILURE;
	}

	log_debug(self->logger, "Personaje %s creado", self->nombre);

	void sigterm_handler(int signum) {
		morir(self, "Muerte por señal");
	}

	void sigusr1_handler(int signum) {
		comer_honguito_verde(self);
	}

	struct sigaction sigterm_action;

	sigterm_action.sa_handler = sigterm_handler;
	sigemptyset(&sigterm_action.sa_mask);
	if (sigaction(SIGTERM, &sigterm_action, NULL ) == -1) {
		log_error(self->logger, "Error al querer setear la señal SIGTERM");
		return EXIT_FAILURE;
	}

	struct sigaction sigusr1_action;

	sigusr1_action.sa_handler = sigusr1_handler;
	sigusr1_action.sa_flags = SA_RESTART;
	sigemptyset(&sigusr1_action.sa_mask);
	if (sigaction(SIGUSR1, &sigusr1_action, NULL ) == -1) {
		log_error(self->logger, "Error al querer setear la señal SIGUSR1");
		return EXIT_FAILURE;
	}

	while (self->plan_de_niveles[self->nivel_actual_index] != NULL ) {

		self->nivel_actual = personaje_nivel_create(
				self->plan_de_niveles[self->nivel_actual_index]);

		log_info(self->logger, "Personaje %s: comenzando nivel %s",
				self->nombre, self->nivel_actual->nombre);

		if (!personaje_conectar_a_orquestador(self)) {
			personaje_destroy(self);
			return EXIT_FAILURE;
		}

		personaje_avisar_fin_de_nivel(self);

		if (!personaje_get_info_nivel(self)) {
			personaje_destroy(self);
			return EXIT_FAILURE;
		}

		log_debug(self->logger,
				"Nivel recibido: NIVEL-IP:%s, NIVEL-PUERTO:%d, PLAN-IP:%s, PLAN-PUERTO:%d",
				self->nivel_actual->nivel->ip,
				self->nivel_actual->nivel->puerto,
				self->nivel_actual->planificador->ip,
				self->nivel_actual->planificador->puerto);

		//Hacer esto es una mierda, pero la otra forma es peor ;)
		log_info(self->logger, "Personaje %s: Desconectando del Orquestador",
				self->nombre);
		if (self->socket_orquestador->serv_socket != NULL ) {
			sockets_destroy(self->socket_orquestador->serv_socket);
		}
		sockets_destroyClient(self->socket_orquestador);
		self->socket_orquestador = NULL;
		//

		if (!personaje_conectar_a_nivel(self)) {
			personaje_destroy(self);
			return EXIT_FAILURE;
		}

		if (!personaje_conectar_a_planificador(self)) {
			personaje_destroy(self);
			return EXIT_FAILURE;
		}

		if (!personaje_jugar_nivel(self)) {
			if (reiniciar_flan) {
				reiniciar_flan = 0;
				self->nivel_actual_index = 0;
				sleep(2);
				log_info(self->logger,
						"Personaje %s: Reiniciando plan de niveles",
						self->nombre);
				continue;
			}

			if (reiniciar_nivel) {
				reiniciar_nivel = 0;
				sleep(2);
				log_info(self->logger, "Personaje %s: Reiniciando nivel",
						self->nombre);
				continue;
			}

			personaje_destroy(self);
			return EXIT_FAILURE;
		}

		self->nivel_actual_index++;
	} //FIN while por cada nivel.

	if (self->plan_de_niveles[self->nivel_actual_index] == NULL ) { //terminé todos los niveles
		//Demian, ya se que está de más, pero me gusta chequearlo al pedo :P
		if (!personaje_conectar_a_orquestador(self)) {
			personaje_destroy(self);
			return false;
		}

		personaje_avisar_fin_de_nivel(self);
		personaje_avisar_fin_de_plan_niveles(self);

		if (self->socket_orquestador->serv_socket != NULL ) {
			sockets_destroy(self->socket_orquestador->serv_socket);
		}
		sockets_destroyClient(self->socket_orquestador);
		self->socket_orquestador = NULL;

		log_info(self->logger, "Personaje %s: Terminé todos los niveles :)",
				self->nombre);
	}
	personaje_destroy(self);

	return EXIT_SUCCESS;
}

bool personaje_get_info_nivel(t_personaje* self) {

	log_info(self->logger,
			"Personaje -> Orquestador: Solicitando datos del nivel %s",
			self->nivel_actual->nombre);

	mensaje_create_and_send(M_GET_INFO_NIVEL_REQUEST,
			strdup(self->nivel_actual->nombre),
			strlen(self->nivel_actual->nombre) + 1, self->socket_orquestador);

	t_mensaje* response = mensaje_recibir(self->socket_orquestador);

	if (response == NULL ) {
		log_error(self->logger,
				"Personaje %s: Error al recibir la información del nivel",
				self->nombre);
		return NULL ;
	}

	if (response->type == M_ERROR) {
		log_error(self->logger, "Orquestador -> Personaje: %s",
				(char*) response->payload);
		mensaje_destroy(response);
		return false;
	}

	if (response->type != M_GET_INFO_NIVEL_RESPONSE) {
		log_error(self->logger,
				"Personaje %s: Error desconocido en la respuesta del Orquestador",
				self->nombre);
		mensaje_destroy(response);
		return false;
	}

	t_get_info_nivel_response* response_data =
			get_info_nivel_response_deserialize((char*) response->payload);

	self->nivel_actual->nivel = response_data->nivel;
	self->nivel_actual->planificador = response_data->planificador;

	free(response_data);
	mensaje_destroy(response);
	return true;
}

t_personaje* personaje_create(char* config_path) {
	t_personaje* new = malloc(sizeof(t_personaje));
	t_config* config = config_create(config_path);

	new->nombre = string_duplicate(config_get_string_value(config, "nombre"));

	char* s = string_duplicate(config_get_string_value(config, "simbolo"));
	new->simbolo = s[0];

	new->plan_de_niveles = config_get_array_value(config, "planDeNiveles");
	new->objetivos = _personaje_load_objetivos(config, new->plan_de_niveles);
	new->vidas = config_get_int_value(config, "vidas");
	new->orquestador_info = connection_create(
			config_get_string_value(config, "orquestador"));

	void morir(char* mensaje) {
		config_destroy(config);
		free(s);
		personaje_destroy(new);
		printf("Error en el archivo de configuración: %s\n", mensaje);
	}

	if (!config_has_property(config, "puerto")) {
		morir("Falta el puerto");
		return NULL ;
	}
	new->puerto = config_get_int_value(config, "puerto");

	char* log_file = "personaje.log";
	char* log_level = "INFO";
	if (config_has_property(config, "logFile")) {
		log_file = string_duplicate(config_get_string_value(config, "logFile"));
	}
	if (config_has_property(config, "logLevel")) {
		log_level = string_duplicate(
				config_get_string_value(config, "logLevel"));
	}
	new->logger = log_create(log_file, "Personaje", true,
			log_level_from_string(log_level));
	config_destroy(config);

	new->socket_orquestador = NULL;
	new->nivel_actual = NULL;
	new->posicion = NULL;
	new->posicion_objetivo = NULL;
	new->nivel_finalizado = false;
	new->nivel_actual_index = 0;
	new->vidas_iniciales = new->vidas;
	new->objetivos_array = NULL;
	new->objetivo_actual = NULL;
	new->objetivo_actual_index = 0;
	new->is_blocked = false;

	free(s);
	free(log_file);
	free(log_level);
	return new;
}

void personaje_destroy(t_personaje* self) {
	free(self->nombre);
	array_destroy(self->plan_de_niveles);
	dictionary_destroy_and_destroy_elements(self->objetivos,
			(void*) array_destroy);
	connection_destroy(self->orquestador_info);
	log_destroy(self->logger);
	if (self->socket_orquestador != NULL ) {
		if (self->socket_orquestador->serv_socket != NULL ) {
			sockets_destroy(self->socket_orquestador->serv_socket);
		}
		sockets_destroyClient(self->socket_orquestador);
	}
	if (self->nivel_actual != NULL ) {
		personaje_nivel_destroy(self->nivel_actual);
	}
	if (self->posicion != NULL ) {
		posicion_destroy(self->posicion);
	}

	if (self->posicion_objetivo != NULL ) {
		posicion_destroy(self->posicion_objetivo);
	}

	if (self->objetivo_actual != NULL ) {
		free(self->objetivo_actual);
	}

	free(self);
}

t_dictionary* _personaje_load_objetivos(t_config* config,
		char** plan_de_niveles) {
	t_dictionary* objetivos = dictionary_create();
	int i = 0;
	while (plan_de_niveles[i] != NULL ) {
		char* nivel = string_duplicate(plan_de_niveles[i]);
		char* key = string_new();
		string_append(&key, "obj[");
		string_append(&key, nivel);
		string_append(&key, "]");
		dictionary_put(objetivos, nivel, config_get_array_value(config, key));
		free(key);
		free(nivel);
		i++;
	}
	return objetivos;
}

bool personaje_conectar_a_orquestador(t_personaje* self) {

	self->socket_orquestador = sockets_conectar_a_servidor(NULL, self->puerto,
			self->orquestador_info->ip, self->orquestador_info->puerto,
			self->logger, M_HANDSHAKE_PERSONAJE, PERSONAJE_HANDSHAKE,
			HANDSHAKE_SUCCESS, "Orquestador");

	if (self->socket_orquestador == NULL ) {
		return false;
	}

	t_mensaje* mensaje = mensaje_recibir(self->socket_orquestador);

	if (mensaje == NULL ) {
		log_error(self->logger,
				"Personaje %s: El orquestador se ha desconectado.",
				self->nombre);
		return false;
	}

	if (mensaje->type != M_GET_SYMBOL_PERSONAJE_REQUEST) {
		mensaje_destroy(mensaje);
		return false;
	}

	mensaje_destroy(mensaje);

	char* simbolo = string_from_format("%c", self->simbolo);

	mensaje_create_and_send(M_GET_SYMBOL_PERSONAJE_RESPONSE,
			string_duplicate(simbolo), strlen(simbolo) + 1,
			self->socket_orquestador);
	free(simbolo);

	return true;
}

t_personaje_nivel* personaje_nivel_create(char* nombre_nivel) {
	t_personaje_nivel* new = malloc(sizeof(t_personaje_nivel));
	new->nombre = string_duplicate(nombre_nivel);
	new->nivel = NULL;
	new->planificador = NULL;
	new->socket_nivel = NULL;
	new->socket_planificador = NULL;
	return new;
}

void personaje_nivel_destroy(t_personaje_nivel* nivel) {

	free(nivel->nombre);

	if (nivel->nivel != NULL ) {
		connection_destroy(nivel->nivel);
	}

	if (nivel->planificador != NULL ) {
		connection_destroy(nivel->planificador);
	}

	if (nivel->socket_planificador != NULL ) {
		if (nivel->socket_planificador->serv_socket != NULL ) {
			sockets_destroy(nivel->socket_planificador->serv_socket);
		}
		sockets_destroyClient(nivel->socket_planificador);
	}

	if (nivel->socket_nivel != NULL ) {
		if (nivel->socket_nivel->serv_socket != NULL ) {
			sockets_destroy(nivel->socket_nivel->serv_socket);
		}
		sockets_destroyClient(nivel->socket_nivel);
	}

	free(nivel);
}

bool verificar_argumentos(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Error en la cantidad de argumentos.\n");
		return false;
	}
	return true;
}

bool personaje_conectar_a_nivel(t_personaje* self) {
	self->nivel_actual->socket_nivel = sockets_conectar_a_servidor(NULL,
			self->puerto, self->nivel_actual->nivel->ip,
			self->nivel_actual->nivel->puerto, self->logger,
			M_HANDSHAKE_PERSONAJE, PERSONAJE_HANDSHAKE, HANDSHAKE_SUCCESS,
			"Nivel");

	if (self->nivel_actual->socket_nivel == NULL ) {
		return false;
	}

	t_mensaje* mensaje = mensaje_recibir(self->nivel_actual->socket_nivel);

	if (mensaje == NULL ) {
		log_error(self->logger, "Personaje %s: El nivel %s se ha desconectado.",
				self->nombre, self->nivel_actual->nombre);
		return false;
	}

	if (mensaje->type != M_GET_SYMBOL_PERSONAJE_REQUEST) {
		mensaje_destroy(mensaje);
		return false;
	}

	mensaje_destroy(mensaje);

	char* simbolo = string_from_format("%c", self->simbolo);

	mensaje_create_and_send(M_GET_SYMBOL_PERSONAJE_RESPONSE,
			string_duplicate(simbolo), strlen(simbolo) + 1,
			self->nivel_actual->socket_nivel);
	free(simbolo);

	if (self->posicion != NULL ) {
		posicion_destroy(self->posicion);
	}
	self->posicion = posicion_create(0, 0);

	return true;
}

bool personaje_conectar_a_planificador(t_personaje* self) {
	self->nivel_actual->socket_planificador = sockets_conectar_a_servidor(NULL,
			self->puerto, self->nivel_actual->planificador->ip,
			self->nivel_actual->planificador->puerto, self->logger,
			M_HANDSHAKE_PERSONAJE, PERSONAJE_HANDSHAKE, HANDSHAKE_SUCCESS,
			"Planificador");

	if (self->nivel_actual->socket_planificador == NULL ) {
		return false;
	}

	t_mensaje* mensaje = mensaje_recibir(
			self->nivel_actual->socket_planificador);

	if (mensaje == NULL ) {
		log_error(self->logger,
				"Personaje %s: El planificador del nivel %s se ha desconectado.",
				self->nombre, self->nivel_actual->nombre);
		return false;
	}

	if (mensaje->type != M_GET_SYMBOL_PERSONAJE_REQUEST) {
		mensaje_destroy(mensaje);
		return false;
	}

	mensaje_destroy(mensaje);

	char* simbolo = string_from_format("%c", self->simbolo);

	mensaje_create_and_send(M_GET_SYMBOL_PERSONAJE_RESPONSE,
			string_duplicate(simbolo), strlen(simbolo) + 1,
			self->nivel_actual->socket_planificador);
	free(simbolo);

	return true;
}

bool personaje_jugar_nivel(t_personaje* self) {
	log_info(self->logger, "Personaje %s: comenzando nivel %s", self->nombre,
			self->nivel_actual->nombre);

	self->objetivos_array = dictionary_get(self->objetivos,
			self->nivel_actual->nombre);

	self->objetivo_actual_index = 0;
	self->objetivo_actual = NULL;

	while (self->objetivos_array[self->objetivo_actual_index] != NULL ) {
		self->objetivo_actual = string_duplicate(
				self->objetivos_array[self->objetivo_actual_index++]); //incremento aca para contabilizar solo cuando cambio de objetivo

		log_info(self->logger, "Personaje %s: nuevo objetivo %s", self->nombre,
				self->objetivo_actual);

		if (self->posicion_objetivo == NULL ) {
			self->posicion_objetivo = pedir_posicion_objetivo(self,
					self->objetivo_actual);
		}

		if (self->posicion_objetivo == NULL ) {
			free(self->objetivo_actual);
			self->objetivo_actual = NULL;
			return false;
		}

		while (!posicion_equals(self->posicion, self->posicion_objetivo)
				|| self->is_blocked) {

			if (!realizar_movimiento(self)) {
				if (reiniciar_nivel || reiniciar_flan) {
					avisar_muerte_a_nivel(self);
					return false;
				}
				free(self->objetivo_actual);
				self->objetivo_actual = NULL;
				return false;
			}

			if (!finalizar_turno(self)) {
				free(self->objetivo_actual);
				self->objetivo_actual = NULL;
				return false;
			}

			if (reiniciar_nivel || reiniciar_flan) {
				avisar_muerte_a_nivel(self);
				return false;
			}

		}
		posicion_destroy(self->posicion_objetivo);
		self->posicion_objetivo = NULL;
		free(self->objetivo_actual);
		self->objetivo_actual = NULL;
	}

	finalizar_nivel(self);

	return true;
}

t_posicion* pedir_posicion_objetivo(t_personaje* self, char* objetivo) {

	log_info(self->logger,
			"Personaje -> Nivel %s: Solicitando posición del recurso %s",
			self->nivel_actual->nombre, objetivo);
	mensaje_create_and_send(M_GET_POSICION_RECURSO_REQUEST,
			string_duplicate(objetivo), strlen(objetivo) + 1,
			self->nivel_actual->socket_nivel);

	t_mensaje* mensaje = mensaje_recibir(self->nivel_actual->socket_nivel);

	if (mensaje == NULL ) {
		log_error(self->logger, "Personaje %s: El nivel %s se ha desconectado.",
				self->nombre, self->nivel_actual->nombre);
		return NULL ;
	}

	t_posicion* posicion_objetivo = posicion_duplicate(mensaje->payload);
	mensaje_destroy(mensaje);

	log_info(self->logger, "Nivel %s -> Personaje: %s esta en (%d,%d)",
			self->nivel_actual->nombre, objetivo, posicion_objetivo->x,
			posicion_objetivo->y);
	return posicion_objetivo;
}

bool realizar_movimiento(t_personaje* self) {
	t_mensaje* notificacion_movimiento = mensaje_recibir(
			self->nivel_actual->socket_planificador);

	if (notificacion_movimiento == NULL ) {
		if (reiniciar_flan || reiniciar_nivel) { //La syscall salió por el EINTR de la señal
			return false;
		}
		log_error(self->logger,
				"Personaje %s: El planificador se ha desconectado.",
				self->nombre);
		return false;
	}

	int mensaje_type = notificacion_movimiento->type;
	mensaje_destroy(notificacion_movimiento);

	if (mensaje_type == M_NOTIFICACION_MOVIMIENTO) {
		log_info(self->logger,
				"Nivel %s -> Personaje: Notificación de movimiento recibida",
				self->nivel_actual->nombre);
		if (posicion_equals(self->posicion, self->posicion_objetivo)
				&& self->is_blocked) {

			log_info(self->logger,
					"Nivel %s -> Personaje: Objetivo %s asignado",
					self->nivel_actual->nombre, self->objetivo_actual);

			if (self->objetivos_array[self->objetivo_actual_index] != NULL ) {
				//Quedan más objetivos, pasamos al siguiente...
				free(self->objetivo_actual);
				self->objetivo_actual = string_duplicate(
						self->objetivos_array[self->objetivo_actual_index++]);

				log_info(self->logger, "Personaje %s: Nuevo objetivo %s",
						self->nombre, self->objetivo_actual);

				posicion_destroy(self->posicion_objetivo);
				self->posicion_objetivo = pedir_posicion_objetivo(self,
						self->objetivo_actual);

				if (self->posicion_objetivo == NULL ) {
					free(self->objetivo_actual);
					self->objetivo_actual = NULL;
					return false;
				}
				return mover_en_nivel(self);
			}
			//Era el último objetivo
			return true;
		}
		return mover_en_nivel(self);
	}

	if (mensaje_type == M_MORITE_GIL) {
		morir(self, "Me muero a pedido del orquestador");
		return false;
	}
	return false;
}

bool mover_en_nivel(t_personaje* self) {
	t_posicion* proxima_posicion = posicion_get_proxima_hacia(self->posicion,
			self->posicion_objetivo);

	log_info(self->logger,
			"Personaje -> Nivel %s: Notificación de movimiento recibida",
			self->nivel_actual->nombre);
	mensaje_create_and_send(M_SOLICITUD_MOVIMIENTO_REQUEST,
			posicion_duplicate(proxima_posicion), sizeof(t_posicion),
			self->nivel_actual->socket_nivel);

	t_mensaje* solicitud_mov_response = mensaje_recibir(
			self->nivel_actual->socket_nivel);

	if (solicitud_mov_response == NULL ) {
		log_error(self->logger, "Personaje %s: El nivel %s se ha desconectado.",
				self->nombre, self->nivel_actual->nombre);
		posicion_destroy(proxima_posicion);
		return false;
	}

	if (solicitud_mov_response->type == M_ERROR) {
		log_error(self->logger,
				"Nivel %s -> Personaje: Movimiento rechazado: %s",
				self->nivel_actual->nombre,
				(char*) solicitud_mov_response->payload);
		mensaje_destroy(solicitud_mov_response);
		posicion_destroy(proxima_posicion);
		return false;
	}

	if (solicitud_mov_response->type == M_SOLICITUD_MOVIMIENTO_OK_RESPONSE) {
		log_info(self->logger, "Nivel %s -> Personaje: Movimiento OK",
				self->nivel_actual->nombre);
		posicion_destroy(self->posicion);
		self->posicion = proxima_posicion;
		mensaje_destroy(solicitud_mov_response);
		return true;
	}
	mensaje_destroy(solicitud_mov_response);
	posicion_destroy(proxima_posicion);
	return false;
}

bool finalizar_turno(t_personaje* self) {

	if (posicion_equals(self->posicion, self->posicion_objetivo)) { //Llegué al recurso

		if (self->is_blocked
				&& self->objetivos_array[self->objetivo_actual_index] == NULL ) {
			//Estaba esperando a que se debloquee el último objetivo
			self->is_blocked = false;
			log_info(self->logger,
					"Personaje -> Planificador Nivel %s: Turno finalizado",
					self->nivel_actual->nombre);
			mensaje_create_and_send(M_TURNO_FINALIZADO_OK, NULL, 0,
					self->nivel_actual->socket_planificador);
			return true;
		}

		log_info(self->logger, "Personaje %s: objetivo %s alcanzado",
				self->nombre, self->objetivo_actual);
		t_mensaje* result = solicitar_recurso(self);

		if (result == NULL ) {
			return false;
		}

		if (result->type == M_SOLICITUD_RECURSO_RESPONSE_OK) {
			log_info(self->logger,
					"Planificador Nivel %s -> Personaje: Objetivo %s asignado",
					self->nivel_actual->nombre, self->objetivo_actual);
			mensaje_create_and_send(M_TURNO_FINALIZADO_SOLICITANDO_RECURSO,
					NULL, 0, self->nivel_actual->socket_planificador);
			mensaje_destroy(result);
			return true;
		}

		if (result->type == M_SOLICITUD_RECURSO_RESPONSE_BLOCKED) {
			log_info(self->logger,
					"Planificador Nivel %s -> Personaje: Objetivo %s NO asignado",
					self->nivel_actual->nombre, self->objetivo_actual);
			self->is_blocked = true;
			log_info(self->logger,
					"Personaje -> Planificador Nivel %s: Turno finalizado bloqueado por el recurso %s",
					self->nivel_actual->nombre, self->objetivo_actual);
			mensaje_create_and_send(M_TURNO_FINALIZADO_BLOCKED,
					string_duplicate(self->objetivo_actual),
					strlen(self->objetivo_actual) + 1,
					self->nivel_actual->socket_planificador);
			mensaje_destroy(result);
			return true;
		}
		mensaje_destroy(result);
		return false;
	} else {
		log_info(self->logger,
				"Personaje -> Planificador Nivel %s: Turno finalizado",
				self->nivel_actual->nombre);
		mensaje_create_and_send(M_TURNO_FINALIZADO_OK, NULL, 0,
				self->nivel_actual->socket_planificador);
		return true;
	}
	return false;
}

t_mensaje* solicitar_recurso(t_personaje* self) {
	log_info(self->logger,
			"Personaje -> Planificador Nivel %s: Turno finalizado",
			self->nivel_actual->nombre);
	mensaje_create_and_send(M_SOLICITUD_RECURSO_REQUEST,
			posicion_duplicate(self->posicion), sizeof(t_posicion),
			self->nivel_actual->socket_nivel);

	t_mensaje* recurso_response = mensaje_recibir(
			self->nivel_actual->socket_nivel);

	return recurso_response;

}

void finalizar_nivel(t_personaje* self) {
	self->nivel_finalizado = true;
	log_info(self->logger,
			"Personaje -> Planificador de Nivel %s: Finalicé el nivel %s",
			self->nivel_actual->nombre, self->nivel_actual->nombre);
	mensaje_create_and_send(M_FIN_DE_NIVEL, NULL, 0,
			self->nivel_actual->socket_nivel);

	limpiar_estado_nivel(self);
}

void morir(t_personaje* self, char* motivo) {
	log_info(self->logger, "Personaje: %s", motivo);
	if (self->vidas > 0) {
		self->vidas--;
		reiniciar_nivel = 1;
	} else {
		self->vidas = self->vidas_iniciales;
		reiniciar_flan = 1;
		log_info(self->logger, "Personaje %s: Reiniciando vidas", self->nombre);
	}
	log_info(self->logger, "Personaje %s: Ahora me quedan %d vidas",
			self->nombre, self->vidas);
}

void personaje_avisar_fin_de_nivel(t_personaje* self) {
	if (self->nivel_finalizado) {
		log_info(self->logger, "Personaje -> Orquestador: Terminé el nivel %s",
				self->nivel_actual->nombre);
		mensaje_create_and_send(M_FIN_DE_NIVEL, NULL, 0,
				self->socket_orquestador);
	}
}

void limpiar_estado_nivel(t_personaje* self) {
	if (self->nivel_actual != NULL ) {
		log_info(self->logger, "Personaje -> Nivel %s: Desconectar",
				self->nivel_actual->nombre);
		log_info(self->logger,
				"Personaje -> Planificador de Nivel %s: Desconectar",
				self->nivel_actual->nombre);
		personaje_nivel_destroy(self->nivel_actual);
		self->nivel_actual = NULL;
	}

	if (self->posicion != NULL ) {
		posicion_destroy(self->posicion);
		self->posicion = NULL;
	}

	if (self->posicion_objetivo != NULL ) {
		posicion_destroy(self->posicion_objetivo);
		self->posicion_objetivo = NULL;
	}

	if (self->socket_orquestador != NULL ) {
		if (self->socket_orquestador->serv_socket != NULL ) {
			sockets_destroy(self->socket_orquestador->serv_socket);
		}
		sockets_destroyClient(self->socket_orquestador);
		self->socket_orquestador = NULL;
	}

	if (self->objetivo_actual != NULL ) {
		free(self->objetivo_actual);
		self->objetivo_actual = NULL;
	}

	self->objetivo_actual_index = 0;
	self->is_blocked = false;
	sleep(2);
}

void avisar_muerte_a_nivel(t_personaje* self) {
	log_info(self->logger, "Personaje -> Nivel %s: Me muero",
			self->nivel_actual->nombre);
	self->nivel_finalizado = false;

	mensaje_create_and_send(M_MUERTE_PERSONAJE, NULL, 0,
			self->nivel_actual->socket_nivel);

	limpiar_estado_nivel(self);
}

void comer_honguito_verde(t_personaje* self) {
	log_info(self->logger,
			"Personaje %s: Llegó un honguito de esos que pegan ;)",
			self->nombre);
	self->vidas++;
	log_info(self->logger, "Personaje %s: Ahora tengo %d vidas", self->nombre,
			self->vidas);
}

void personaje_avisar_fin_de_plan_niveles(t_personaje* self) {
	if (self->nivel_finalizado) {
		log_info(self->logger,
				"Personaje %s: Notificando plan de niveles finalizado a orquestador.",
				self->nombre);

		char* str_simbolo = string_from_format("%c", self->simbolo);
		mensaje_create_and_send(M_FIN_PLAN_DE_NIVELES,
				string_duplicate(str_simbolo), strlen(str_simbolo) + 1,
				self->socket_orquestador);
		free(str_simbolo);
	}
}
