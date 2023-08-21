#ifndef KERNEL_H
#define KERNEL_H

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/txt.h>
#include <estructuras.h>
#include "planificador_largo.h"
#include "planificador_corto.h"
#include "i_cpu.h"
#include "i_file_system.h"

extern t_kernel_config* kernel_config;

// SE CREAN COMO ESTRUCTURAS. SI SE UTILIZAN EN HILOS RECORDAR DE PASAR COMO REFERENCIA CON -> &
pthread_t hilo_plp; //PLANIFICADOR DE LARGO PLAZO
pthread_t hilo_pcp; // PLANIFICADOR DE CORTO PLAZO
pthread_t hilo_kernel_cpu; // MANEJO DE INTERFAZ CON LA CPU
pthread_t hilo_kernel_fs; // MANEJO DE INTERFAZ CON FILE SYSTEM
/* -- VARIABLES -- */
int socket_cpu;
extern int socket_filesystem;
extern int socket_memoria;
int socket_kernel;
int socket_consola;
extern t_list* procesos_en_kernel;

/* -- FUNCIONES -- */
int conectar_con_cpu();
int conectar_con_memoria();
int conectar_con_filesystem();
void cargar_config_kernel();
void finalizar_kernel(int , t_log* , t_config*);
int procesar_consola(void *);

#endif
