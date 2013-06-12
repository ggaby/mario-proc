#include <commons/collections/list.h>
#include <stdbool.h>

int posicionActual; //Lo necesitas por Next-Fit
int tamanioTotal; // Ver si es peronista tenerlo aca, o en el c

typedef char* t_memoria;

typedef struct t_particion{
	char id;
	int inicio;
	int tamanio;
	t_memoria dato;
	bool ocupado;
} particion;

t_memoria crearMemoria();

int almacenarParticion();

int eliminarParticion();

void liberarMemoria();

t_list* particiones;
