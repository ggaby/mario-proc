#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include "../Plataforma.h"
#include "../../common/sockets.h"
#include "../../common/mensaje.h"
#include <commons/collections/queue.h>

#define PLANIFICADOR_CONFIG_QUANTUM "quantum"
#define PLANIFICADOR_CONFIG_TIEMPO_ESPERA "tiempoEntreMovimientos"

void* planificador(void* plataforma);
t_planificador* planificador_create(t_plataforma* plataforma,
		t_connection_info* connection_info, char* nombre_nivel);
void planificador_personaje_destroy(planificador_t_personaje* self);
void planificador_destroy(t_planificador* self);
planificador_t_personaje* planificador_personaje_create(t_socket_client* socket);
bool planificador_esta_procesando(t_planificador* self);
void planificador_resetear_quantum(t_planificador* self);
bool planificador_process_request(t_planificador* self, t_mensaje* mensaje,
		t_plataforma* plataforma);
void planificador_finalizar_turno(t_planificador* self);
void planificador_mover_personaje(t_planificador* self);
void planificador_cambiar_de_personaje(t_planificador* self);

#endif /* PLANIFICADOR_H_ */
