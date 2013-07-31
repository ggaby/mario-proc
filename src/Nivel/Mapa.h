
#ifndef MAPA_H_
#define MAPA_H_

#include <nivel-gui/nivel.h>
#include <curses.h>

typedef struct{
	ITEM_NIVEL* items;
	int colums;
	int rows;
}t_mapa;

t_mapa* mapa_create(int columns, int rows); //Logger(?)
void mapa_destroy(t_mapa* self);
void mapa_dibujar(t_mapa* self);
void mapa_create_personaje(t_mapa* self, char id_personaje);
void mapa_create_caja_recurso(t_mapa* self, char id_caja, int posx, int posy, int cantidad);
void mapa_update_recurso(t_mapa* self, char id_caja, int cantidad);
bool mapa_mover_personaje(t_mapa* self, char id_personaje, int xdestino, int ydestino);
void mapa_borrar_item(t_mapa* self, char id);

#endif /* MAPA_H_ */
