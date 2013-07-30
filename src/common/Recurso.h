/*
 * Recurso.h
 *
 *  Created on: 29/07/2013
 *      Author: reyiyo
 */

#ifndef RECURSO_H_
#define RECURSO_H_

#include <stdbool.h>
#include "../common/posicion.h"

typedef struct {
	char* nombre;
	char simbolo;
	int cantidad;
	t_posicion* posicion;
} t_recurso;

t_recurso* recurso_create(char* nombre, char simbolo, int cantidad,
		t_posicion* posicion);
t_recurso* recurso_from_config_string(char* config_string);
void recurso_destroy(t_recurso* recurso);
t_recurso* recurso_clone(t_recurso* recurso);
bool recurso_equals(t_recurso* one, t_recurso* other);

#endif /* RECURSO_H_ */
