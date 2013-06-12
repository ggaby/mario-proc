/*
 * list.h
 *
 *  Created on: 10/06/2013
 *      Author: reyiyo
 */

#ifndef MY_LIST_H_
#define MY_LIST_H_

#include <commons/collections/list.h>

void my_list_remove_and_destroy_by_condition(t_list *self,
		bool (*condition)(void*), void (*element_destroyer)(void*));

#endif /* MY_LIST_H_ */
