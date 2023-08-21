#ifndef SRC_ESTRUCTURAS_H_
#define SRC_ESTRUCTURAS_H_
#include <commons/collections/list.h>
#include <commons/temporal.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <commons/collections/queue.h>
#include <commons/config.h>

/*
 * GENERAL
 * */

typedef enum
{
	// GENERAL
	MENSAJE = 1,
	PAQUETE,
	// HANDSHAKES
	KERNEL = 10,
	CPU,
	FILESYSTEM,
	CONSOLA,
	// CONSOLA-KERNEL
	PROGRAMA = 20,
	PROGRAMA_FINALIZADO,
	// KERNEL-CPU
	CONTEXTO_PROCESO = 30, //TODO
	PROCESO_DESALOJADO_POR_YIELD,
	PROCESO_FINALIZADO,
	PROCESO_BLOQUEADO,
	PROCESO_DESALOJADO_POR_WAIT,
	PROCESO_DESALOJADO_POR_SIGNAL,
	PROCESO_DESALOJADO_POR_F_OPEN,
	PROCESO_DESALOJADO_POR_F_CLOSE,
	PROCESO_DESALOJADO_POR_F_SEEK,
	PROCESO_DESALOJADO_POR_F_READ,
	PROCESO_DESALOJADO_POR_F_WRITE,
	PROCESO_DESALOJADO_POR_F_TRUNCATE,
	PROCESO_DESALOJADO_POR_CREATE_SEGMENT,
	PROCESO_DESALOJADO_POR_DELETE_SEGMENT,
	PROCESO_DESALOJADO_POR_SEG_FAULT,
    
	//KERNEL - MEMORIA
	MEMORY_CREATE_TABLE = 60,
	MEMORY_DELETE_TABLE,
	MEMORY_CREATE_SEGMENT, //62
	MEMORY_DELETE_SEGMENT,
	MEMORY_COMPACT,
	MEMORY_SEGMENT_CREATED,
	MEMORY_SEGMENT_DELETED, //66
	MEMORY_SEGMENT_TABLE_CREATED, //67
	MEMORY_SEGMENT_TABLE_UPDATED,
	MEMORY_SEGMENT_TABLE_DELETED, //69
	MEMORY_SEG_FAULT, // 70
	MEMORY_ERROR_OUT_OF_MEMORY,
	MEMORY_ERROR_TABLE_NOT_FOUND,
	MEMORY_NEEDS_TO_COMPACT,
	//CPU-FS-MEMORIA
	MEMORY_READ_ADRESS,
	MEMORY_WRITE_ADRESS
} op_code;

typedef enum
{
	// RETURN CODES DE KERNEL A CONSOLA
	SUCCESS = 1,
	SEG_FAULT,
	OUT_OF_MEMORY,
	NOT_DEFINED,
	RESOURCE_NOT_FOUND
} return_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef struct {
	int socket;
	int socket_cpu;
	t_log* log;
	pthread_mutex_t* mutex;
} t_args_hilo_cliente;

typedef struct {
	t_log* log;
	t_config* config;
} t_args_hilo_planificador;

typedef struct {
	t_queue* cola;
	pthread_mutex_t* mutex;
} t_squeue;

/*			// (void*)pcb,
			// tiempo_bloqueo,
			// (void*) algoritmo*/


/*CODIGO DE INSTRUCCION*/
typedef enum {
	ci_SET = 1,
	ci_WAIT,
	ci_SIGNAL,
	ci_YIELD,
	ci_IO,
	ci_F_OPEN,
	ci_F_READ,
	ci_F_WRITE,
	ci_F_TRUNCATE,
	ci_F_SEEK,
	ci_F_CLOSE,
	ci_CREATE_SEGMENT,
	ci_DELETE_SEGMENT,
	ci_MOV_IN,
	ci_MOV_OUT,
	ci_EXIT
} t_codigo_instruccion;

typedef enum {
	F_OPEN,
	F_CREATE,
	F_TRUNCATE,
	F_READ,
	F_WRITE,
	F_NOT_EXISTS,
	F_EXISTS,
	F_OP_OK,
	F_OPEN_OK,
	F_TRUNCATE_OK,
	F_READ_OK,
	F_WRITE_OK,
	F_OP_ERROR
} t_codigo_operacionfs;

typedef struct {
	t_codigo_instruccion codigo;
	t_list* parametros;
} t_instruccion;


typedef struct {
	int size;
	t_list* instrucciones;
} t_programa;

// REGISTROS CPU
typedef struct {
    char AX[5];
    char BX[5];
    char CX[5];
    char DX[5];
    char EAX[9];
    char EBX[9];
    char ECX[9];
    char EDX[9];
    char RAX[17];
    char RBX[17];
    char RCX[17];
    char RDX[17];
} t_registro;

typedef struct{
	int pid;
	int program_counter;
	t_list* instrucciones;
	t_registro registros;
	t_list* tabla_segmentos;
}t_contexto_proceso;

/*
 * KERNEL - MEMORIA
 * */

typedef struct {
	uint32_t descriptor_id;
	uint32_t segmento_id;
	uint32_t inicio;
	uint32_t tam_segmento;
}t_segmento;

t_instruccion* crear_instruccion(t_codigo_instruccion, bool);
void buffer_destroy(t_buffer*);
t_buffer* serializar_instrucciones(t_list* instrucciones, t_log* logger);
t_list* deserializar_instrucciones(t_buffer* buffer, t_log* logger);
void enviar_contexto(int socket,t_contexto_proceso* contexto,int codigo,t_log* logger);
t_contexto_proceso* recibir_contexto(int socket,t_log* logger);
int size_of_registros(t_contexto_proceso* contexto);

t_buffer* serializar_tabla_segmentos(t_list* tabla_segmentos);
t_list* deserializar_tabla_segmentos(void* stream);
void enviar_tabla_de_segmentos(int socket,t_list* tabla_segmentos);
t_list* recibir_tabla_de_segmentos(int socket);
void loggear_segmentos(t_list* lista_segmentos, t_log* logger);

#endif /* SRC_ESTRUCTURAS_H_ */
