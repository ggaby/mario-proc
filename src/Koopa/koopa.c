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

int funcion_parser(t_parser parser[], char* archivo)  
{
 FILE* piConf;
 char* cvBuffLine = (char*)malloc(2048+1);
 char* pcProp=(char*)malloc(2048+1);
 char* auxString=(char*)malloc(20+1);

 int linea=0;
   
 if((piConf = fopen(archivo, "rt")) == NULL)
 {
    return ERROR;
 }
 while(fgets(cvBuffLine,2048,piConf))
 {
    pcProp=NULL;
    linea++;
    if(cvBuffLine[0] != '#') /* Si no es linea comentada*/
    {
        pcProp = (char *)strtok(cvBuffLine,": \n\0\b\f\r\t\v");
        if(pcProp != NULL)
        {
            strcpy(parser[linea]->operacion,pcProp);
            //parser[linea]->operacion[strlen(pcProp+1)] = '\0'; /* TODOv er si es necesario*/
                  
            if ((pcProp = (char *)strtok(NULL,": \n\0\b\f\r\t\v")) != NULL)
            {
                strcpy(parser[linea]->id,pcProp);
                            
                if ((pcProp = (char *)strtok(NULL,": \n\0\b\f\r\t\v")) != NULL)
                {
                    strcpy(auxString,pcProp);
                    sscanf(auxString, "%d", &parser[linea]->tamano);
                    
                    if ((pcProp = (char *)strtok(NULL,": \n\0\b\f\r\t\v")) != NULL)
                    {
                        strcpy(parser[linea]->contenido,pcProp);
                        printf("lei cadena linea %d",linea);
                    
                    }
                }
                        
            }
        }
    }
 }
 free (cvBuffLine);
 free (pcProp);
 free (auxString);
 fclose (piConf);
 return 1; //TODO ver que devuelvo.

}
