#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "memoria.h"

typedef struct {
	int index;
	t_particion* particion;
} t_fragmento;

t_list* listaParticiones;

int last_pos_segmento; //Lo necesitas por Next-Fit
int libre;
int actual_index_particiones;
int tamanioSegmento;

int crear_particion_nueva(t_memoria segmento, char id, int tamanio,
		char* contenido, bool ocupado);
t_fragmento* buscar_la_primera_que_entre(int tamanio);
t_fragmento* buscar_elemento(char id);
int guardar_datos(t_fragmento* guardar_aca, t_memoria segmento, char id,
		char* contenido);
t_fragmento* fragmento_create(char id, int tamanio, int inicio, bool ocupado);
void fragmento_destroy(t_fragmento* self);
void crear_particion_vacia_en_posicion(t_memoria segmento, char id, int tamanio, bool ocupado, int posicion);
t_fragmento* buscar_elemento_existente(int inicio);

t_memoria crear_memoria(int tamanio) {
	tamanioSegmento = tamanio;
	libre = tamanio;
	last_pos_segmento = 0;
	actual_index_particiones = 0;
	listaParticiones = list_create();
	char* memoria = malloc(tamanio);
	return memoria;
}

int almacenar_particion(t_memoria segmento, char id, int tamanio,
		char* contenido) {

	t_fragmento* existe = buscar_elemento(id);
	if (existe != NULL || tamanio >= tamanioSegmento) {
		return -1;
	}
	t_fragmento* existeElIndice = buscar_elemento_existente(last_pos_segmento);

	if (tamanio <= libre && existeElIndice == NULL) {
		return crear_particion_nueva(segmento, id, tamanio, contenido, false);
	} else {
		t_fragmento* guardar_aca = buscar_la_primera_que_entre(tamanio);
		if (guardar_aca == NULL ) {
			return 0; //No hay Memoria
		}
		return guardar_datos(guardar_aca, segmento, id, contenido);
	}
}

int eliminar_particion(t_memoria segmento, char id) {
	t_fragmento* eliminar = buscar_elemento(id);
	if (eliminar == NULL ) {
		return 0;
	}
	eliminar->particion->libre = true;
	t_fragmento* siguiente = list_get(listaParticiones, eliminar->index+1);
	if (siguiente == NULL){
		last_pos_segmento = eliminar->particion->inicio;
	}
	return 1;
}

void liberar_memoria(t_memoria segmento) {
	free(segmento);
	list_destroy_and_destroy_elements(listaParticiones,
			(void*) fragmento_destroy);
}

t_list* particiones(t_memoria segmento) {
	t_particion* get_particion(t_fragmento* elem) {
		return elem->particion;
	}

	bool _esMayor(t_particion* elem1, t_particion* elem2){
		return elem1->inicio < elem2->inicio;
	}
	t_list *listaPantalla = list_map(listaParticiones, (void*) get_particion);
	list_sort(listaPantalla, (void*) _esMayor);
	if (libre > 0) {
		t_particion* ultimaParticion = malloc(sizeof(t_particion));
		ultimaParticion->id='0';
		ultimaParticion->inicio=tamanioSegmento-libre;
		ultimaParticion->tamanio=libre;
		ultimaParticion->libre=true;
		char* lugar_del_segmento = segmento + (tamanioSegmento-libre);
		ultimaParticion->dato = lugar_del_segmento;
		list_add(listaPantalla, ultimaParticion);
	}
	return listaPantalla;
}

int crear_particion_nueva(t_memoria segmento, char id, int tamanio,
		char* contenido, bool vacio) {

	t_fragmento* nuevo = fragmento_create(id, tamanio, last_pos_segmento, vacio);
	char* lugar_del_segmento = segmento + last_pos_segmento;
	nuevo->particion->dato = lugar_del_segmento;
	memcpy(nuevo->particion->dato, contenido, tamanio);
	last_pos_segmento += tamanio;
	actual_index_particiones = list_add(listaParticiones, nuevo);
	nuevo->index = actual_index_particiones;
	libre -= tamanio;
	return 1; //Todo joya
}

void crear_particion_vacia_en_posicion(t_memoria segmento, char id, int tamanio, bool vacio, int posicion) {

	t_fragmento* nuevo = fragmento_create(id, tamanio, posicion, vacio);
	char* lugar_del_segmento = segmento + posicion;
	nuevo->particion->dato = lugar_del_segmento;
	last_pos_segmento += tamanio;
	actual_index_particiones = list_add(listaParticiones, nuevo);
	nuevo->index = actual_index_particiones;
}

t_fragmento* buscar_la_primera_que_entre(int tamanio) {

	bool entra(t_fragmento* elem) {
		return ((elem->particion->tamanio >= tamanio) && elem->particion->libre);
	}

	bool es_siguiente(t_fragmento* elem) {
		return elem->index >= actual_index_particiones;
	}

	t_list* temp = list_filter(listaParticiones, (void*) es_siguiente);
	t_fragmento* aca_entra = list_find(temp, (void*) entra);
	if (aca_entra == NULL ) {
		aca_entra = list_find(listaParticiones, (void*) entra);
	}
	return aca_entra;

}

t_fragmento* buscar_elemento(char id) {
	bool id_existe(t_fragmento* elem) {
		return (elem->particion->id == id && !elem->particion->libre);
	}
	t_fragmento* existe = list_find(listaParticiones, (void*) id_existe);
	return existe;
}

int guardar_datos(t_fragmento* guardar_aca, t_memoria segmento, char id,
		char* contenido) {
	t_fragmento* elem = fragmento_create(id, strlen(contenido), guardar_aca->particion->inicio, false);
	elem->particion->dato= segmento + guardar_aca->particion->inicio;
	memcpy(elem->particion->dato, contenido, strlen(contenido));
	elem->index = guardar_aca->index;
	int viejoTamanio = guardar_aca->particion->tamanio-elem->particion->tamanio;
	int nuevoInicio = guardar_aca->particion->inicio +strlen(contenido);
	char idParticionVacia = tolower(guardar_aca->particion->id);
	actual_index_particiones = guardar_aca->index;
	list_replace_and_destroy_element(listaParticiones, guardar_aca->index, elem,(void*) fragmento_destroy);
	last_pos_segmento = guardar_aca->particion->inicio +strlen(contenido);
	if(viejoTamanio !=0){
		crear_particion_vacia_en_posicion(segmento, idParticionVacia, viejoTamanio, true, nuevoInicio);
	}
	return 1;
}

t_fragmento* fragmento_create(char id, int tamanio, int inicio, bool vacio) {
	t_particion* particion = malloc(sizeof(t_particion));
	particion->id = id;
	particion->tamanio = tamanio;
	particion->libre = vacio;
	particion->inicio = inicio;

	t_fragmento* new = malloc(sizeof(t_fragmento));
	new->index = -1;
	new->particion = particion;
	return new;
}

void fragmento_destroy(t_fragmento* self) {
	free(self->particion);
	free(self);
}

t_fragmento* buscar_elemento_existente(int inicio){
	bool _existe(t_fragmento* elem) {
		return (elem->particion->inicio == inicio);
	}
		t_fragmento* existe = list_find(listaParticiones, (void*)_existe);
		return existe;
}


