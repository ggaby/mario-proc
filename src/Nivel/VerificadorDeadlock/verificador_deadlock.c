#include "verificador_deadlock.h"
#include "../Nivel.h"

void* verificador_deadlock(void* level) {

	nivel_t_nivel* nivel = (nivel_t_nivel*) level;

	nivel_loguear(log_info, nivel, "Verificador de Deadlock creado, tiempo de checkeo: %d", nivel->tiempoChequeoDeadlock);

	return (void*) EXIT_SUCCESS;
}

