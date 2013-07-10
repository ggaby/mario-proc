#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include "../../common/sockets.h"
#include "../../common/mensaje.h"
#include <commons/collections/queue.h>

#define PLANIFICADOR_CONFIG_QUANTUM "quantum"
#define PLANIFICADOR_CONFIG_TIEMPO_ESPERA "tiempoEntreMovimientos"

void* planificador(void* plataforma);

typedef struct{
	t_socket_client* socket;
}planificador_t_personaje;

typedef struct{
	t_queue* personajes_ready;
	t_queue* personajes_blocked;
	int quantum_total;
	int quantum_actual;
	unsigned int tiempo_entre_movimientos;
}t_planificador;

t_planificador* planificador_create(char* config_path);
void planificador_destroy(t_planificador* self);
planificador_t_personaje* planificador_create_add_personaje(t_planificador* self, t_socket_client* socket);
bool planificador_esta_procesando(t_planificador* self);
void planificador_resetear_quantum(t_planificador* self);
bool planificador_process_request(t_planificador* self, t_mensaje* mensaje);
void planificador_finalizar_turno(t_planificador* self);
void planificador_mover_personaje(t_planificador* self, planificador_t_personaje* personaje);
void planificador_cambiar_de_personaje(t_planificador* self);

#endif /* PLANIFICADOR_H_ */
