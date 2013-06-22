/*
 * common_structs.c
 *
 *  Created on: 04/05/2013
 *      Author: reyiyo
 */

#include "common_structs.h"
#include <stdlib.h>

void array_destroy(char** array) {
	int i = 0;
	while (array[i] != NULL ) {
		free(array[i]);
		i++;
	}
	free(array);
}
