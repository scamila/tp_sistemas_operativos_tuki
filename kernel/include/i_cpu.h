#ifndef I_CPU_H_
#define I_CPU_H_

#include "planificador_corto.h"

extern int socket_cpu;
extern int socket_memoria;
extern t_colas* colas_planificacion;
extern sem_t cpu_liberada;
extern sem_t proceso_enviado;
extern sem_t request_file_system;
extern pthread_mutex_t mutex_socket_memoria;
extern pthread_mutex_t puede_compactar;
extern sem_t f_seek_done;
extern sem_t f_close_done;
extern sem_t f_open_done;
extern t_list* lista_recursos;
extern t_list* archivos_abiertos;

void manejar_respuesta_cpu(void* args_hilo);

void pasar_a_ready_segun_algoritmo(char* algoritmo,t_pcb* proceso,t_log* logger);

void actualizar_pcb(t_pcb* pcb, t_contexto_proceso* contexto);
void procesar_contexto(t_pcb* pcb, op_code cod_op, char* algoritmo, t_log* logger);

void bloqueo_io(void* vArgs);
void procesar_wait_recurso(char* nombre,t_pcb* pcb,char* algoritmo,t_log* logger);
void procesar_signal_recurso(char* nombre,t_pcb* pcb,char* algoritmo,t_log* logger);
void procesar_f_open(t_pcb* pcb);
void procesar_f_close(t_pcb* pcb);
void procesar_f_seek(t_pcb* pcb);
void procesar_f_read(t_pcb* pcb);
void procesar_f_write(t_pcb* pcb);
void procesar_f_truncate(t_pcb* pcb);
void procesar_create_segment(t_pcb* pcb);
void procesar_delete_segment(t_pcb* pcb);

void solicitar_eliminar_tabla_de_segmento(t_pcb* pcb);

void ejectuar_f_seek(int pid, char* nombre_archivo, int posicion_puntero);
void ejecutar_f_open(t_pcb* pcb, char* nombre_archivo);

#endif
