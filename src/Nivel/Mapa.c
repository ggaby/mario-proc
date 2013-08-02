#include "Mapa.h"
#include <stdlib.h>

void mapa_createItem(ITEM_NIVEL** listaItems, char id, int x , int y, char tipo, int cant_rec);

t_mapa* mapa_create(){
	nivel_gui_inicializar();

	t_mapa* self = malloc(sizeof(t_mapa));
	self->items = NULL;
	nivel_gui_get_area_nivel(&self->colums, &self->rows);
	return self;
}

void mapa_destroy(t_mapa* self){
	nivel_gui_terminar();

	ITEM_NIVEL* aux;
	while(self->items != NULL){
		aux = self->items;
		self->items = aux->next;
		free(aux);
	}

	free(self);
}

void mapa_dibujar(t_mapa* self){
	nivel_gui_dibujar(self->items);
}

void mapa_create_personaje(t_mapa* self, char id_personaje){
	mapa_createItem(&self->items, id_personaje, 0, 0, PERSONAJE_ITEM_TYPE, 0);
}

void mapa_create_caja_recurso(t_mapa* self, char id_caja, int posx, int posy, int cantidad){
	mapa_createItem(&self->items, id_caja, posx, posy, RECURSO_ITEM_TYPE, cantidad);
}

void mapa_update_recurso(t_mapa* self, char id_caja, int cantidad){
	ITEM_NIVEL * temp;
	temp = self->items;

	while ((temp != NULL) && (temp->id != id_caja)) {
		temp = temp->next;
	}

	if ((temp != NULL) && (temp->id == id_caja)) {
		if ((temp->item_type == RECURSO_ITEM_TYPE)) { //TODO las condisiones se pueden meter en el if de arriba.
			temp->quantity = cantidad;
		}
	}
}

bool mapa_mover_personaje(t_mapa* self, char id_personaje, int xdestino, int ydestino) {

	if(!mapa_contiene(self, xdestino, ydestino)){
		return false;
	}

	ITEM_NIVEL * temp = self->items;

	while ((temp != NULL) && (temp->id != id_personaje)) {
		temp = temp->next;
	}
	if ((temp != NULL) && (temp->id == id_personaje) && (temp->item_type == PERSONAJE_ITEM_TYPE)) {
		temp->posx = xdestino;
		temp->posy = ydestino;
		return true;
	}

	return false;
}

void mapa_borrar_item(t_mapa* self, char id){
	ITEM_NIVEL * temp = self->items;
	ITEM_NIVEL * oldtemp;

	if ((temp != NULL) && (temp->id == id)) {
		self->items = self->items->next;
		free(temp);
	}
	else {
		while((temp != NULL) && (temp->id != id)) {
				oldtemp = temp;
				temp = temp->next;
		}
		if ((temp != NULL) && (temp->id == id)) {
				oldtemp->next = temp->next;
				free(temp);
		}
	}
}

//-- Private Methods--//

void mapa_createItem(ITEM_NIVEL** listaItems, char id, int x , int y, char tipo, int cant_rec) {
        ITEM_NIVEL * temp;
        temp = malloc(sizeof(ITEM_NIVEL));

        temp->id = id;
        temp->posx=x;
        temp->posy=y;
        temp->item_type = tipo;
        temp->quantity = cant_rec;
        temp->next = *listaItems;
        *listaItems = temp;
}

bool mapa_contiene(t_mapa* self, int x, int y){
	return 0 <= x && x <= self->rows && 0 <= y && y <= self->colums;
}
