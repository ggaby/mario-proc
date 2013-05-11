/*
 * common_structs.h
 *
 *  Created on: 24/04/2013
 *      Author: reyiyo
 */

#ifndef COMMON_STRUCTS_H_
#define COMMON_STRUCTS_H_

	typedef struct {
		char* ip;
		int puerto;
	} t_connection_info;

	t_connection_info* t_connection_new(char* ip_y_puerto);
	void t_connection_destroy(t_connection_info* self);
	void array_destroy(char** array);

#endif /* COMMON_STRUCTS_H_ */
