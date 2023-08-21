#ifndef PLANIFICADOR_CORTO_H_
#define PLANIFICADOR_CORTO_H_

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/txt.h>
#include <estructuras.h>
#include "planificador_utils.h"


extern t_colas* colas_planificacion;
extern sem_t sem_grado_multiprogramacion;
extern sem_t sem_nuevo_proceso;
extern sem_t sem_ready_proceso;
extern sem_t sem_exec_proceso;

extern sem_t cpu_liberada;

extern pthread_mutex_t mutex_cola_ready;

extern t_kernel_config* kernel_config;

int planificador_corto_plazo(void*);
t_pcb* planificar(char* algoritmo, t_log* logger);
t_pcb* proximo_proceso_hrrn(t_log* logger);
void loggear_registros(t_registro registro, t_log* logger);
bool comparador_hrrn(void* pcb1, void* pcb2);

#endif /* PLANIFICADOR_CORTO_H_ */
