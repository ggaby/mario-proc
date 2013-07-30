#include "verificador_deadlock.h"
#include <unistd.h>
#include "../../common/list.h"
#include <commons/string.h>

bool hay_espera_circular(t_list* personajes_bloqueados);
void loguear_deadlock_detectado(nivel_t_nivel* nivel, t_list* personajes_bloqueados);

void* verificador_deadlock(void* level) {

	nivel_t_nivel* nivel = (nivel_t_nivel*) level;

	nivel_loguear(log_info, nivel,
			"Verificador de Deadlock creado, tiempo de checkeo: %d",
			nivel->tiempoChequeoDeadlock);

	while (true) {

		nivel_loguear(log_info, nivel,
				"Verificador de Deadlock esperando %d segundos para realizar el checkeo.",
				nivel->tiempoChequeoDeadlock);
		sleep(nivel->tiempoChequeoDeadlock);

		t_list* recursos_disponibles = clonar_recursos(nivel);
		t_list* personajes_bloqueados = nivel_get_personajes_bloqueados(nivel);

		bool hay_deadlock = false;

		bool verificar_personaje_en_deadlock(nivel_t_personaje* personaje) {
			bool es_el_recurso(t_recurso* recurso) {
				return recurso->simbolo
						== personaje->simbolo_recurso_esperado[0];
			}

			t_recurso* recurso = list_find(recursos_disponibles,
					(void*) es_el_recurso);

			if (recurso->cantidad > 0) {
				liberar_recursos_del_personaje(personaje, recursos_disponibles);
				hay_deadlock = false;
				return true;
			}

			return false;
		}

		while (!hay_deadlock && list_size(personajes_bloqueados) > 1) { //1 solo bloqueado nunca puede estar en deadlock
			hay_deadlock = true;
			my_list_remove_and_destroy_by_condition(personajes_bloqueados,
					(void*) verificar_personaje_en_deadlock,
					(void*) nivel_destroy_personaje);
		}

		if (hay_deadlock && hay_espera_circular(personajes_bloqueados)) {
			//TODO En caso de que varios esten esperando el recurso que tenga tomado un unico personaje que puede terminar su ejecucion esto tira deadlock pero no es cierto... hay que verificar espera circular entre los procesos restantes?

			loguear_deadlock_detectado(nivel, personajes_bloqueados);

			if(nivel->recovery){
				//TODO avisar al orquestador, etc...
			}

		} else {
			nivel_loguear(log_info, nivel,
					"Verificador deadlock: NO HAY DEADLOCK");
		}

		list_destroy_and_destroy_elements(recursos_disponibles,
				(void*) recurso_destroy);

		list_destroy_and_destroy_elements(personajes_bloqueados,
				(void*) nivel_destroy_personaje);

	}

	return (void*) EXIT_SUCCESS;
}

bool hay_espera_circular(t_list* personajes_bloqueados){
	//TODO falto de ideas para ver como hacer esto :S
	return true;
}

void loguear_deadlock_detectado(nivel_t_nivel* nivel, t_list* personajes_bloqueados){
	char* ids_personajes_en_deadlock = string_new();

	void appendear_id_personaje(nivel_t_personaje* personaje) {
		char* id_personaje = string_from_format("%c ", personaje->id);
		string_append(&ids_personajes_en_deadlock, id_personaje);
		free(id_personaje);
	}


	list_iterate(personajes_bloqueados, (void*) appendear_id_personaje);

	nivel_loguear(log_info, nivel,
			"Verificador deadlock: DEADLOCK DETECTADO, personajes involucrados: %s",
			ids_personajes_en_deadlock);

	free(ids_personajes_en_deadlock);
}

void liberar_recursos_del_personaje(nivel_t_personaje* personaje,
		t_list* recursos_disponibles) {

	void liberar_recurso(t_recurso* recurso) {
		bool es_el_recurso(t_recurso* elem) {
			return recurso_equals(recurso, elem);
		}

		t_recurso* mi_recurso = list_find(recursos_disponibles,
				(void*) es_el_recurso);
		mi_recurso->cantidad += recurso->cantidad;
	}

	list_iterate(personaje->recursos_asignados, (void *) liberar_recurso);

	free(personaje->simbolo_recurso_esperado);
	personaje->simbolo_recurso_esperado = NULL;
}

t_list* nivel_get_personajes_bloqueados(nivel_t_nivel* nivel) {
	t_list* personajes_bloqueados = list_create();

	void clonar_personaje_bloqueado(nivel_t_personaje* personaje) {
		if (personaje->simbolo_recurso_esperado != NULL ) {

			nivel_t_personaje* new = malloc(sizeof(nivel_t_personaje));

			new->id = personaje->id;

			new->simbolo_recurso_esperado = string_duplicate(
					personaje->simbolo_recurso_esperado);

			new->recursos_asignados = my_list_clone_and_clone_elements(
					personaje->recursos_asignados, (void*) recurso_clone);

			new->posicion = NULL;

			list_add(personajes_bloqueados, new);
		}
	}

	list_iterate(nivel->personajes, (void*) clonar_personaje_bloqueado);
	return personajes_bloqueados;
}

t_list* clonar_recursos(nivel_t_nivel* nivel) {
	return my_list_clone_and_clone_elements(nivel->recursos,
			(void*) recurso_clone);
}
