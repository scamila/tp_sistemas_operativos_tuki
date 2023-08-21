#ifndef CPU_H_
#define CPU_H_

#include "ciclo_de_instruccion.h"

typedef struct{
    int retardo_instruccion;
    char* ip_memoria;
    char* puerto_memoria;
    char* puerto_escucha;
    int tam_max_segmento;
} t_cpu_config;

typedef struct{
    char registros_4[4][5];
    char registros_8[4][9];
    char registros_16[4][17];
} t_reg;

void terminar(void);
void cargar_config(char* path);
void conexion_a_memoria(char* ip,char* puerto,t_log* logger);
void correr_servidor(void);
void loggear_registros(t_registro* registro);
void actualizar_registros_pcb(t_registro* registros);
void setear_registros_desde_proceso(t_contexto_proceso* proceso);
void limpiar_registros_cpu(int tam,char registro[][tam]);
void liberar_proceso(t_contexto_proceso* proceso);
void liberar_parametros_instruccion(void* instruccion);

#endif /* CPU_H_ */
