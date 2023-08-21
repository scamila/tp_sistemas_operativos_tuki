#ifndef SRC_PLANIFICADOR_UTILS_H_
#define SRC_PLANIFICADOR_UTILS_H_

#include <estructuras.h>
#include <errno.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdio.h>
#include <pthread.h>
#include <shared.h>

typedef enum{
	NEW,
	READY,
	EXEC,
	BLOCK,
	BLOCK_RECURSO,
	EXIT
} t_estado;

// PCB
typedef struct {
	int pid;
	t_list* instrucciones;
	int program_counter;
	t_registro registros;
	int estimado_rafaga;
	int nuevo_estimado;
	t_temporal* tiempo_llegada;
	t_temporal* tiempo_ejecucion;
	t_estado estado_actual;
	t_list* tabla_archivos_abiertos;
	t_list* tabla_segmento;
	return_code motivo;
	sem_t sem_exit_proceso;
} t_pcb;

typedef struct {
	t_squeue* cola_ready;
	t_squeue* cola_new;
	t_squeue* cola_exec;
	t_squeue* cola_block;
	t_squeue* cola_exit;
	t_squeue* cola_archivos;
	t_squeue* log_ejecucion;
} t_colas;

typedef struct {
    char* pids;
} ConcatenacionPIDs;

typedef struct {
	char* nombre;
	int instancias;
	t_squeue* cola_bloqueados;
} t_recurso;

typedef struct {
	t_pcb* pcb;
	int tiempo_bloqueo;
	char* algoritmo;
	t_log* logger;
} t_args_hilo_block_io;

/* -- ESTRUCTURAS -- */
typedef struct
{
	char* IP_MEMORIA;
	char* PUERTO_MEMORIA;
	char* IP_FILESYSTEM;
	char* PUERTO_FILESYSTEM;
	char* IP_CPU;
	char* PUERTO_CPU;
	char* PUERTO_ESCUCHA;
    char* ALGORITMO_PLANIFICACION;
    int ESTIMACION_INICIAL;
    int HRRN_ALFA;
    int GRADO_MAX_MULTIPROGRAMACION;
    char** RECURSOS;
    char** INSTANCIAS_RECURSOS;

} t_kernel_config;

typedef struct {
    int file_id;
    char* nombre;
    int puntero;
    int cant_aperturas;
    t_squeue* cola_bloqueados;
    pthread_mutex_t* mutex;
} t_archivo_abierto;

typedef struct{
	t_pcb* pcb;
	int direccion_fisica;
} proceso_fs;

void iniciar_colas_planificacion(void);
void destroy_colas_planificacion(void);
void iniciar_semaforos(int);
void destroy_semaforos(void);
t_pcb* crear_pcb(t_programa*, int);
void destroy_pcb(t_pcb*);
/*
 * Crea una estructura t_contexto en base a un pcb y lo envía al cpu
 * */
void ejecutar_proceso(int, t_pcb*, t_log*);
/*
 * Quita el PCB de La cola Actual, y lo pasa a la cola de READY
 * Activa Timer de espera
 * */
void pasar_a_cola_ready(t_pcb*, t_log*);
/*
 * Quita el PCB de La cola READY, y lo pasa a la cola de EXEC
 * Activa Timer de ejecución
 * */
void pasar_a_cola_ready_en_orden(t_pcb* pcb_nuevo, t_log* logger, int(*comparador)(t_pcb*, t_pcb*, t_log*));
void pasar_a_cola_exec(t_pcb*, t_log*);
void pasar_a_cola_blocked(t_pcb* pcb, t_log* logger, t_squeue* cola);
void pasar_a_cola_exit(t_pcb*, t_log*, return_code);
void ejecutar_f_close(t_pcb* pcb, char* nombre_archivo);
void cerrar_archivos_asociados(t_pcb* pcb);
char* concatenar_pids(t_list*);
void loggear_cola_ready(t_log* logger, char* algoritmo);
char* estado_string(int);
t_registro crear_registro(void);
t_temporal* temporal_reset(t_temporal* temporal);

void iniciar_recursos(char** recursos, char** instancias);
int buscar_recurso(char* nombre);
double calcular_estimado_proxima_rafaga (t_pcb* pcb, t_log* logger);

t_squeue* squeue_create(void);
void squeue_destroy(t_squeue* queue);
void* squeue_pop(t_squeue* queue);
void squeue_push(t_squeue* queue, void* element);
void* squeue_peek(t_squeue* queue);

void procesar_respuesta_memoria(t_pcb *pcb);
t_segmento* recibir_segmento(void);
t_list* recibir_tabla_segmentos(int socket);


void sincronizar_tablas_procesos(void);
void reenviar_create_segment(t_pcb* pcb);

void loggear_tablas_archivos(void);
t_archivo_abierto* obtener_archivo_abierto(char* nombre_archivo);
t_archivo_abierto* crear_archivo_abierto(char* nombre_archivo);
void archivo_abierto_destroy(t_archivo_abierto* archivo);
t_pcb* buscar_pcb_en_lista(int pid, t_list* lista);

#endif /* SRC_PLANIFICADOR_UTILS_H_ */
