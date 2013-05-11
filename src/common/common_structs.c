/*
 * common_structs.c
 *
 *  Created on: 04/05/2013
 *      Author: reyiyo
 */

#include "common_structs.h"
#include <stddef.h>
#include <stdlib.h>
#include <commons/string.h>

t_connection_info* t_connection_new(char* ip_y_puerto) {
	t_connection_info* new = malloc(sizeof(t_connection_info));
	char** tokens = string_split(ip_y_puerto, ":");
	new->ip = string_duplicate(tokens[0]);
	new->puerto = atoi(tokens[1]);
	array_destroy(tokens);
	return new;
}

void t_connection_destroy(t_connection_info* self) {
	free(self->ip);
	free(self);
}

void array_destroy(char** array) {
	int i = 0;
	while (array[i] != NULL ) {
		free(array[i]);
		i++;
	}
	free(array);
}
