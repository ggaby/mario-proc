/*
 * Plataforma.h
 *
 *  Created on: 22/06/2013
 *      Author: test
 */

#ifndef PLATAFORMA_H_
#define PLATAFORMA_H_

#include <pthread.h>
#include <commons/log.h>

typedef struct {
	t_log* logger;
	pthread_mutex_t logger_mutex;
} t_plataforma;


t_plataforma* plataforma_create();
void plataforma_destroy(t_plataforma* self);

#endif /* PLATAFORMA_H_ */
