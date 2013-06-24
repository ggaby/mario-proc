/*
 * mensaje.h
 *
 *  Created on: 02/06/2013
 *      Author: reyiyo
 */

#ifndef MENSAJE_H_
#define MENSAJE_H_

#include <stdint.h>
#include <stdlib.h>
#include "sockets.h"
#include <string.h>

#define M_HANDSHAKE 1
#define M_ERROR 2

#define M_HANDSHAKE_PERSONAJE "He aqui un personaje"
#define DATA_NIVEL_HANDSHAKE "He aqui un nivel"

//______________MENSAJES_DEL_NIVEL_Y_ORQUESTADOR____________________//
/*El nivel se conecta con el orquesador como cliente para:
	Enviar un mensaje:
	1) Librera recursos: pueden ser por 3 casos:
		a)muerte de un personaje, por signal o porque el contador de vida menor a cero.
		b)personaje gano el nivel.
		c)se desconecto un personaje.
	2) Estoy en deadlock, solucionalo
	*/
#define DATA_NIVEL_ORQUESTADOR_LIBERA_RECURSOS "Libera recursos"  //se puede pasar un personaje
#define DATA_NIVEL_ORQUESTADOR_DEADLOCK "Estoy en deadlock"
	/*
	Recibir un mensaje:
	1) Recursos asignados.
	2) Seleccion de victima, si tiene el recovery activado el nivel.
*/
#define DATA_NIVEL_ORQUESTADOR_RECURSOS_ASIGNADOS "Recursos asignados"
#define DATA_NIVEL_ORQUESTADOR_VICTIMA "VICTIMA"  //se puede pasar un personaje

//______________MENSAJES_DEL_NIVEL_Y_ORQUESTADOR____________________//

//______________MENSAJES_DEL_NIVEL_Y_PERSONAJE____________________//

/*El nivel se conecta con el personaje como servidor para:
	Recibir un mensaje del personaje:
	1) damePosicionDelMapa()
	2) dameUnMovimiento()
	3) dameRecurso()
	4) estoyBloqueado()
	5) estoyMuerto()
*/

//______________MENSAJES_DEL_NIVEL_Y_PERSONAJE____________________//


typedef struct t_stream {
	int length;
	void *data;
} t_stream;

typedef struct t_mensaje {
	uint8_t type;
	uint16_t length;
	void* payload;
}__attribute__ ((packed)) t_mensaje;

t_mensaje *mensaje_create(uint8_t type);
void mensaje_setdata(t_mensaje* mensaje, void* data, uint16_t length);
void* mensaje_getdata(t_mensaje* mensaje);
void mensaje_destroy(t_mensaje* mensaje);
void* mensaje_getdata_destroy(t_mensaje* mensaje);
t_socket_buffer* mensaje_serializer(t_mensaje* mensaje);
t_mensaje* mensaje_deserializer(t_socket_buffer* buffer, uint32_t dataStart);
t_mensaje* mensaje_clone(t_mensaje* mensaje);
void mensaje_send(t_mensaje* mensaje, t_socket_client *client);

#endif /* MENSAJE_H_ */
