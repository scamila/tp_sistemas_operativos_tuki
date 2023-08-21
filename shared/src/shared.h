#ifndef SHARED_H_
#define SHARED_H_

#include "estructuras.h"
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/string.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>
#include<signal.h>

extern t_log* logger;
/*
 * SERVIDOR
 * */
int iniciar_servidor(char*);
int esperar_cliente(int,t_log*);
void enviar_handshake(int, int);
void enviar_entero(int socket, int valor);
int recibir_operacion(int);
int recibir_entero(int socket);
int recibir_entero_2(int socket);
void* recibir_buffer(int*, int);
void recibir_mensaje(int,t_log*); // DEPRECADO?
char * recibir_string(int socket_cliente);
t_list* recibir_paquete(int, t_log*);
t_buffer* recibir_datos(int socket);
void* serializar_paquete(t_paquete* paquete, int bytes);
/*
 * CLIENTE
 * */
int crear_conexion(char* ip, char* puerto);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_buffer* crear_buffer();
t_paquete* crear_paquete(int);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_buffer(t_buffer* buffer);
void eliminar_paquete(t_paquete* paquete);
t_config* iniciar_config(char*);

void agregar_int_a_paquete(t_paquete* paquete, int valor);
int extraer_int(t_buffer* buffer);
char* extraer_string(t_buffer* buffer, int* tamanio);
char* obtener_string(t_buffer* buffer);

/*
 * GENERAL
 * */

t_log* iniciar_logger(char*);
void terminar_programa(int, t_log*, t_config*);
t_programa* crear_programa(t_list*);
void programa_destroy(t_programa*);
int validar_conexion(int socket);
char* nombre_de_instruccion(int cod_op);

#endif /* SHARED_H_ */
