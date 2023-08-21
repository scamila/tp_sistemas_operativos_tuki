#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <commons/config.h>
#include <commons/txt.h>
#include <commons/collections/queue.h>
#include "estructuras.h"
#include "file_parser.h"

#define PATH_LOG "consola.log"

void correr_consola(char*, char*);
void terminar_programa(int, t_log*, t_config*);
int conexion_a_kernel(char*, char*, t_log*);
/*
 *	Tipo de datos			Tamaño			Descripcion
 *	int 					4 bytes			Cantidad de instrucciones
 *		int					4 bytes			Codigo de instruccion
 *		int					4 bytes			Cantidad de parametros
 *			int				4 bytes			Longitud del parametro - incluye al centinela de fin de cadena "\n"
 *			char*			variable		Valor del parametro - 1 byte por caracter + centinela
 */

t_buffer* serializar_programa(t_programa*, t_log*);

int enviar_programa(t_buffer*, int, int, t_log*);
#endif /* CONSOLA_H_ */
