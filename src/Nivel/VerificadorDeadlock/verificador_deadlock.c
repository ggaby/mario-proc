#include "verificador_deadlock.h"
#include <unistd.h>
#include "../../common/list.h"
#include <commons/string.h>

void loguear_deadlock_detectado(nivel_t_nivel* nivel,
		t_list* personajes_bloqueados);

void avisar_deadlock_al_orquestador(nivel_t_nivel* nivel,
		t_list* personajes_bloqueados);

char* personajes_as_string_ids(t_list* personajes);

void* verificador_deadlock(void* level) {

	nivel_t_nivel* nivel = (nivel_t_nivel*) level;

	nivel_loguear(log_info, nivel,
			"Verificador de Deadlock creado, tiempo de checkeo: %.2f segundos",
			nivel->tiempoChequeoDeadlock / 1000000.0);

	while (true) {

		nivel_loguear(log_info, nivel,
				"Verificador de Deadlock esperando %.2f segundos para realizar el checkeo.",
				nivel->tiempoChequeoDeadlock / 1000000.0);
		usleep(nivel->tiempoChequeoDeadlock);

		t_list* recursos_disponibles = clonar_recursos(nivel);
		t_list* personajes_bloqueados = clonar_personajes(nivel);

		bool hay_deadlock = false;

		bool personaje_puede_ejecutar(nivel_t_personaje* personaje) {
			if (personaje->simbolo_recurso_esperado == NULL ) {
				liberar_recursos_del_personaje(personaje, recursos_disponibles);
				hay_deadlock = false;
				return true;
			}

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

		while (!hay_deadlock && !list_is_empty(personajes_bloqueados)) {
			hay_deadlock = true;
			my_list_remove_and_destroy_by_condition(personajes_bloqueados,
					(void*) personaje_puede_ejecutar,
					(void*) nivel_destroy_personaje);
		}

		if (hay_deadlock) {
			loguear_deadlock_detectado(nivel, personajes_bloqueados);

			if (nivel->recovery) {
				avisar_deadlock_al_orquestador(nivel, personajes_bloqueados);
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

void loguear_deadlock_detectado(nivel_t_nivel* nivel,
		t_list* personajes_bloqueados) {

	char* ids_personajes_en_deadlock = personajes_as_string_ids(
			personajes_bloqueados);

	nivel_loguear(log_info, nivel,
			"Verificador deadlock: DEADLOCK DETECTADO, personajes involucrados: %s",
			ids_personajes_en_deadlock);

	free(ids_personajes_en_deadlock);
}

void avisar_deadlock_al_orquestador(nivel_t_nivel* nivel,
		t_list* personajes_bloqueados) {

	char* ids_personajes = personajes_as_string_ids(personajes_bloqueados);

	mensaje_create_and_send(M_DEADLOCK_DETECTADO,
			string_duplicate(ids_personajes), strlen(ids_personajes)+1,
			nivel->socket_orquestador);

	free(ids_personajes);
}

char* personajes_as_string_ids(t_list* personajes) {
	char* ids_personajes = string_new();

	void appendear_id_personaje(nivel_t_personaje* personaje) {
		char* id_personaje = string_from_format("%c ", personaje->id);
		string_append(&ids_personajes, string_duplicate(id_personaje));
		free(id_personaje);
	}

	list_iterate(personajes, (void*) appendear_id_personaje);

	string_trim(&ids_personajes);
	return ids_personajes;
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

}

t_list* clonar_personajes(nivel_t_nivel* nivel) {
//	t_list* personajes_bloqueados = list_create();

	nivel_t_personaje* clonar_personaje(nivel_t_personaje* personaje) {
		nivel_t_personaje* new = malloc(sizeof(nivel_t_personaje));
		new->id = personaje->id;

		if (personaje->simbolo_recurso_esperado == NULL ) {
			new->simbolo_recurso_esperado = NULL;
		} else {
			new->simbolo_recurso_esperado = string_duplicate(
					personaje->simbolo_recurso_esperado);
		}

		new->recursos_asignados = my_list_clone_and_clone_elements(
				personaje->recursos_asignados, (void*) recurso_clone);
		new->posicion = NULL;

		return new;
	}

//	list_iterate(nivel->personajes, (void*) clonar_personaje_bloqueado);
//	return personajes_bloqueados;
	return my_list_clone_and_clone_elements(nivel->personajes,
			(void*) clonar_personaje);
}

t_list* clonar_recursos(nivel_t_nivel* nivel) {
	return my_list_clone_and_clone_elements(nivel->recursos,
			(void*) recurso_clone);
}
