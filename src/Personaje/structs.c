/*
 * structs.c
 *
 *  Created on: 24/04/2013
 *      Author: reyiyo
 */

#include "../common/common_structs.h"
#include <commons/config.h>
#include <commons/collections/dictionary.h>
#include <commons/string.h>
#include <stddef.h>
#include <stdlib.h>
#include "structs.h"
#include <stdio.h>

t_dictionary* _personaje_load_objetivos(t_config* config,
			char** plan_de_niveles);

t_personaje* personaje_new(char* config_path) {
	t_personaje* new = malloc(sizeof(t_personaje));
	t_config* config = config_create(config_path);
	new->nombre = config_get_string_value(config, "nombre");
	char* s = config_get_string_value(config, "simbolo");
	new->simbolo = s[0];
	new->plan_de_niveles = config_get_array_value(config, "planDeNiveles");
	new->objetivos = _personaje_load_objetivos(config, new->plan_de_niveles);
	new->vidas = config_get_int_value(config, "vidas");
	new->orquestador = t_connection_new(
			config_get_string_value(config, "orquestador"));
	config_destroy(config);
	free(s);
	return new;
}

void personaje_destroy(t_personaje* self) {
	free(self->nombre);
	array_destroy(self->plan_de_niveles);
	dictionary_destroy_and_destroy_elements(self->objetivos, (void*) array_destroy);
	t_connection_destroy(self->orquestador);
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

t_nivel* nivel_new(t_connection_info* nivel, t_connection_info* planificador){
	t_nivel* new = malloc(sizeof(t_nivel));
	new->data = nivel;
	new->planificador = planificador;
	return new;
}

void nivel_destroy(t_nivel* self) {
	t_connection_destroy(self->data);
	t_connection_destroy(self->planificador);
	free(self);
}
