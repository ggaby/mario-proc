/*
 * list.c
 *
 *  Created on: 10/06/2013
 *      Author: reyiyo
 */

#include <stddef.h>
#include "list.h"
/*
 * @NAME: my_list_remove_and_destroy_by_condition
 * @DESC: Remueve y destruye los elementos de la lista que hagan que condition devuelva != 0.
 */

void my_list_remove_and_destroy_by_condition(t_list *self,
		bool (*condition)(void*), void (*element_destroyer)(void*)) {
	int flag = 1;
	while (flag) {
		void* data = list_remove_by_condition(self, condition);
		if (data != NULL ) {
			element_destroyer(data);
		} else {
			flag = 0;
		}
	}
}
