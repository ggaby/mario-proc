#ifndef VERFICADOR_DEADLOCK_H_
#define VERFICADOR_DEADLOCK_H_

#include <commons/collections/list.h>
#include "../Nivel.h"

void* verificador_deadlock(void* nivel);

t_list* clonar_recursos(nivel_t_nivel* nivel);
t_list* nivel_get_personajes_bloqueados(nivel_t_nivel* nivel);
void liberar_recursos_del_personaje(nivel_t_personaje* personaje,
		t_list* recursos_disponibles);

#endif /* VERFICADOR_DEADLOCK_H_ */
