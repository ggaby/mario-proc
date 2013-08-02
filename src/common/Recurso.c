/*
 * Recurso.c
 *
 *  Created on: 29/07/2013
 *      Author: reyiyo
 */

#include "Recurso.h"
#include <commons/string.h>
#include "../common/common_structs.h"

t_recurso* recurso_create(char* nombre, char simbolo, int cantidad,
		t_posicion* posicion) {
	t_recurso* new = malloc(sizeof(t_recurso));
	if (nombre != NULL) {
		new->nombre = string_duplicate(nombre);
	} else {
		new->nombre = NULL;
	}
	new->simbolo = simbolo;
	new->cantidad = cantidad;
	new->posicion = posicion;
	return new;
}

t_recurso* recurso_from_config_string(char* config_string) {
	//config_string example: Caja1=Flores,F,3,23,5
	char** values = string_split(config_string, ",");

	t_recurso* new = recurso_create(values[0], values[1][0], atoi(values[2]),
			posicion_create(atoi(values[3]), atoi(values[4])));

	array_destroy(values);
	return new;
}

void recurso_destroy(t_recurso* self) {
	if (self->nombre != NULL) {
		free(self->nombre);
	}
	if (self->posicion != NULL) {
		posicion_destroy(self->posicion);
	}
	free(self);
}

//Sí, recurso otra vez (?)
//FIXME Esto de hacerle chistes malos al código es muy triste...
t_recurso* recurso_clone(t_recurso* recurso) {
	return recurso_create(recurso->nombre, recurso->simbolo, recurso->cantidad,
			posicion_duplicate(recurso->posicion));
}

bool recurso_equals(t_recurso* one, t_recurso* other) {
	return one->simbolo == other->simbolo;
}
