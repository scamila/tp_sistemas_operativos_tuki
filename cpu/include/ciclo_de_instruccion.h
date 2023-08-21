#ifndef CICLO_DE_INSTRUCCION_H
#define CICLO_DE_INSTRUCCION_H

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <estructuras.h>
#include <commons/config.h>
#include "cpu.h"

void ciclo_de_instruccion(t_contexto_proceso* proceso,int socket);
void execute_set(char* registro, char* valor);
void execute_mov_in(int direccion_fisica, char* registro,int direccion_logica);
void execute_mov_out(int direccion_fisica, int direccion_logica, char* registro);
void execute_io(int tiempo);
void execute_f_open(char* nombre_archivo);
void execute_f_close(char* nombre_archivo);
void execute_f_seek(char* nombre_archivo, int posicion);
void execute_f_read(char* nombre_archivo, int direccion_logica, int cant_bytes);
void execute_f_write(char* nombre_archivo, int direccion_logica, int cant_bytes);
void execute_f_truncate(char* nombre_archivo, int tamanio);
void execute_wait(char* recurso);
void execute_signal(char* recurso);
void execute_create_segment(int id_segmento, int tamanio);
void execute_delete_segment(int id_segmento);
void execute_yield(void);
void execute_exit(void);

int traducir_a_direccion_fisica(int direccion_logica , t_contexto_proceso* proceso, int cant_bytes);
t_segmento* obtener_segmento(int direccion_logica,t_list* tabla_segmentos);

char* leer_memoria(int direccion_fisica, int cant_de_bytes);
void escribir_memoria(int direccion_fisica, char* valor_a_escribir, int tamanio);

void set_valor_registro(char* nombre_registro,char* valor);
char* get_valor_registro(char* nombre_registro);
int posicion_registro(char* nombre_registro);

void devolver_proceso(t_contexto_proceso*,int,t_log*);

#endif
