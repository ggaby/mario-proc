#include "posicion.h"
#include <stdlib.h>

t_posicion* posicion_create(x, y){
	t_posicion* new = malloc(sizeof(t_posicion));
	new->x = x;
	new->y = y;

	return new;
}

void posicion_destroy(t_posicion* self){
	free(self);
}

t_posicion* posicion_duplicate(t_posicion* posicion){
	return posicion_create(posicion->x, posicion->y);
}

bool posicion_equals(t_posicion* self, t_posicion* other){
	return self->x == other->x && self->y == other->y;
}

t_posicion* posicion_get_proxima_hacia(t_posicion* self, t_posicion* posicion_destino){

	if(posicion_equals(self, posicion_destino)){
		return posicion_duplicate(self);
	}

	int distancia_a_mover = DISTANCIA_MOVIMIENTO_PERMITIDA;

	int x = self->x;
	int y = self->y;

	//TODO hay logica repetida... despues veremos...
	if (x != posicion_destino->x) {
		if (x < posicion_destino->x) {
			x += distancia_a_mover;
		} else {
			x -= distancia_a_mover;
		}
	} else if (y != posicion_destino->y) {
		if (y < posicion_destino->y) {
			y += distancia_a_mover;
		} else {
			y -= distancia_a_mover;
		}
	}

	return posicion_create(x,y);
}

int posicion_get_distancia(t_posicion* self, t_posicion* otra_posicion){
	return abs(self->x - otra_posicion->x) + abs(self->y - otra_posicion->y);
}
