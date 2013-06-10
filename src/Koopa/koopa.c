/*
 * koopa.c
 *
 *  Created on: 15/05/2013
 *      Author: perezoscluis
 */

#include  "koopa.h"
#include <stdlib.h>
#include <stddef.h>

int fragmentacion;

int main(int argc, char* argv[]) {
	t_memoria crearMemoria();
	//TODO: Ampliar el main (Llama al parser, y while banderita)
	liberarMemoria();
}

//Creo la memoria con una particion que a va a ser loteada a medida que vaya
//necesitandose

t_memoria crearMemoria() {
	printf("Ingrese el numero de la memoria a utilizar");
	scanf("%d", tamanioTotal);
	particiones = list_create();
	particion *particionNueva;
	particionNueva->id = 'A'; //TODO: Ver el primer id;
	particionNueva->inicio = 0;
	particionNueva->ocupado = false;
	particionNueva->tamanio = tamanioTotal;
	particionNueva->dato = (t_memoria) malloc(tamanioTotal);
	int errno = list_add(particiones, particionNueva);
	if (errno != 0) {
		return NULL ;
	} else {
		printf("Se creo la particion");
		posicionActual = 0;
		return NULL ; //Ver si/que tendria que devolver
	}
}

void liberarMemoria() {
	list_clean(particiones);
	free(particiones);
}

int almacenarParticion() {
	particion *particionNueva;
	char letra;
	int tamanio;
	scanf("%c", letra);
	scanf("%d", tamanio); //Datos del parser;
	fragmentacion = 0;
	while (fragmentacion < 2) {
		if (posicionActual + tamanio <= tamanioTotal) {
			particionNueva->id = letra;
			particionNueva->inicio = posicionActual;
			particionNueva->ocupado = true;
			particionNueva->tamanio = tamanio;
			particionNueva->dato = (t_memoria) malloc(tamanio);
			int errno = list_add(particiones, particionNueva);
			if (errno != 0) {
				return errno;
			} else {
				printf("Se creo la particion ");
				posicionActual = posicionActual + tamanio;
				return errno; //Ver si/que tendria que devolver
			}
		} else {
			posicionActual = 0;
			fragmentacion++;
			if (fragmentacion == 2){
				printf("Error de fragmentacion");
				return NULL;
			}

		}
	}
}

int eliminarParticion(){
	char idAMatar;
	int errno = list_remove_by_condition(particiones, (idAMatar));
	return errno;
}
