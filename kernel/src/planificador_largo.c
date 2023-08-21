#include "../include/planificador_largo.h"

int planificador_largo_plazo(void* args_hilo) {

	t_args_hilo_planificador* args = (t_args_hilo_planificador*) args_hilo;
	t_log* logger = args->log;
	//t_config* config = args->config;
	log_info(logger, "P_LARGO -> Inicializado Hilo Planificador de Largo Plazo");

	while(1) {
		log_debug(logger, "P_LARGO -> Esperando wait de nuevo proceso");
		sem_wait(&sem_nuevo_proceso);
		log_debug(logger, "P_LARGO -> Esperando wait de multiprogramación");
		sem_wait(&sem_grado_multiprogramacion);
		t_pcb* pcb = (t_pcb*)squeue_peek(colas_planificacion->cola_new);
		log_debug(logger, "P_LARGO -> Se va a aceptar un nuevo proceso %d", pcb->pid);
		solicitar_nueva_tabla_de_segmento(pcb);
		pasar_a_cola_ready(pcb, logger);
	}

	return 1;
}

void solicitar_nueva_tabla_de_segmento(t_pcb* pcb) {
	pthread_mutex_lock(&mutex_socket_memoria);
	validar_conexion(socket_memoria);
	log_debug(logger, "P_LARGO -> Solicitando Tabla de Segmentos para PID: %d...", pcb->pid);

	t_paquete* paquete = crear_paquete(MEMORY_CREATE_TABLE);
	paquete->buffer = crear_buffer();
	agregar_int_a_paquete(paquete, pcb->pid);
	enviar_paquete(paquete, socket_memoria);
	//RECV
	procesar_respuesta_memoria(pcb);
	pthread_mutex_unlock(&mutex_socket_memoria);
}

void loggear_tabla(t_pcb* pcb, char* origen) {
	log_info(logger, "%s -> Tabla de PCB %d", origen, pcb->pid);
	int cant_segmentos = list_size(pcb->tabla_segmento);
	//log_info(logger, "%s -> La cantidad de segmentos es: %d", origen, cant_segmentos);
	for (int i = 0; i < cant_segmentos; i++) {
		t_segmento* segmento = list_get(pcb->tabla_segmento, i);
		log_info(logger, "%s -> Segmento ID: %d, Inicio: %d, Tamaño: %d", origen, segmento->segmento_id, segmento->inicio, segmento->tam_segmento);
	}
}
