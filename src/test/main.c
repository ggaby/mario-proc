/*
 * main.c
 *
 *  Created on: 12/06/2013
 *      Author: reyiyo
 */

/*
 * TODO: Super TODO: Hacerlo con CUnit!
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../common/list.h"
#include <commons/string.h>

typedef struct {
	char* algo;
	int borrar;
} t_test_struct;

t_test_struct* t_test_struct_create(char* algo, int borrar) {
	t_test_struct* new = malloc(sizeof(t_test_struct));
	new->algo = string_duplicate(algo);
	new->borrar = borrar;
	return new;
}

void t_test_struct_destroy(t_test_struct* self) {
	free(self->algo);
	free(self);
}

int main(int argc, char* argv[]) {
	t_test_struct* elem1 = t_test_struct_create("elem1", 1);
	t_test_struct* elem2 = t_test_struct_create("elem2", 0);
	t_test_struct* elem3 = t_test_struct_create("elem3", 1);

	t_list* la_lista = list_create();
	list_add(la_lista, elem1);
	list_add(la_lista, elem2);
	list_add(la_lista, elem3);

	bool es_para_borrar(t_test_struct* elem) {
		return elem->borrar == 1;
	}

	printf("Cantidad de elementos: %d\n", list_size(la_lista));

	my_list_remove_and_destroy_by_condition(la_lista,
			(void*) es_para_borrar, (void*) t_test_struct_destroy);

	//Debería quedar 1;
	printf("Cantidad de elementos después del borrado: %d\n", list_size(la_lista));

	list_destroy_and_destroy_elements(la_lista, (void*) t_test_struct_destroy);

	return EXIT_SUCCESS;
}

