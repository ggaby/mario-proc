#include <stdlib.h>
#include <string.h>

#include "memoria.h"

typedef struct {
	int index;
	t_particion* particion;
} t_fragmento;

int crear_particion_nueva(t_memoria segmento, char id, int tamanio,
		char* contenido);
t_fragmento* buscar_la_primera_que_entre(int tamanio);
t_fragmento* buscar_elemento(char id);
int guardar_datos(t_fragmento* guardar_aca, t_memoria segmento, char id,
		char* contenido);
t_fragmento* fragmento_create(char id, int tamanio, int inicio);
void fragmento_destroy(t_fragmento* self);


t_list* listaParticiones;

int last_pos_segmento; //Lo necesitas por Next-Fit
int libre;
int actual_index_particiones;
int tamanioSegmento;

/*void llenarBuraco(int inicio, int tamanio) {
 t_particion *nuevaParticion = (t_particion*) malloc(sizeof(t_particion));
 nuevaParticion->inicio = inicio;
 nuevaParticion->libre = true;
 nuevaParticion->tamanio = tamanio;
 list_add_in_index(listaParticiones, inicio ,(void*)nuevaParticion);
 }*/

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

	if (tamanio <= libre) {
		return crear_particion_nueva(segmento, id, tamanio, contenido);
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
	return 0;
}

void liberar_memoria(t_memoria segmento) {
	free(segmento);
	list_destroy_and_destroy_elements(listaParticiones, (void*) fragmento_destroy);
}

t_list* particiones(t_memoria segmento) {
	t_particion* get_particion(t_fragmento* elem){
		return elem->particion;
	}

	if (libre > 0) {
		crear_particion_nueva(segmento, '0', libre, "");
	}
	t_list *listaPantalla = list_map(listaParticiones, (void*) get_particion);
	return listaPantalla;
}

int crear_particion_nueva(t_memoria segmento, char id, int tamanio,
		char* contenido) {

	t_fragmento* nuevo = fragmento_create(id, tamanio, last_pos_segmento);
	char* lugar_del_segmento = segmento + last_pos_segmento;
	strcpy(lugar_del_segmento, contenido);
	last_pos_segmento += tamanio;
	actual_index_particiones = list_add(listaParticiones, nuevo);
	nuevo->index = actual_index_particiones;
	libre -= tamanio;
	return 1; //Todo joya
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
	guardar_aca->particion->id = id;
	guardar_aca->particion->libre = false;

	char* lugar_del_segmento = segmento + guardar_aca->particion->inicio;
	strcpy(lugar_del_segmento, contenido);
	actual_index_particiones = guardar_aca->index;
	return 1;
}

t_fragmento* fragmento_create(char id, int tamanio, int inicio) {
	t_particion* particion = malloc(sizeof(t_particion));
	particion->id = id;
	particion->tamanio = tamanio;
	particion->libre = false;
	particion->inicio = inicio;

	t_fragmento* new = malloc(sizeof(t_fragmento));
	new->index = -1;
	new->particion = particion;
	return new;
}

void fragmento_destroy(t_fragmento* self) {
	free(self->particion->dato);
	free(self->particion);
	free(self);
}

