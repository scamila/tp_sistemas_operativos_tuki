#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <shared.h>
#include <commons/config.h>
#include <commons/txt.h>
#include "estructuras.h"
#include "memoria_utils.h"

extern t_espacio_usuario* espacio_usuario;
extern t_list* tablas_segmentos;
extern t_memoria_config* memoria_config;

extern t_log * logger;
extern int id;

int escuchar_clientes(int server_fd, t_log *logger);
void procesar_cliente(void *args_hilo);
void procesar_kernel(int socket_cliente);
void procesar_cpu_fs(int socket_cliente, char* modulo);
void enviar_tabla_segmento(int socket_kernel, t_tabla_segmento* tabla_segmento, int cod_op);
void enviar_tabla_actualizada(int socket_kernel, int pid, int cod_op);

void enviar_procesos_actualizados(int socket);
#endif
