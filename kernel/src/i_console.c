#include "../include/i_console.h"
void procesar_consola(void *args_hilo) {

	t_args_hilo_cliente *args = (t_args_hilo_cliente*) args_hilo;

	int socket_consola = args->socket;
	t_log* logger = args->log;
	int socket_cpu = args->socket_cpu;

	log_info(logger, "Iniciado nuevo Hilo de escucha con consola en socket %d", socket_consola);

	int estado_socket = validar_conexion(socket_consola);
	int cod_op = recibir_operacion(socket_consola);

	switch (cod_op) {
		case PROGRAMA:
			t_buffer* buffer = recibir_buffer_programa(socket_consola, logger);
			t_programa* programa = deserializar_programa(buffer, logger);
//			loggear_programa(programa,logger);
			t_pcb* proceso = crear_proceso(programa, logger, socket_cpu);
			respuesta_proceso(proceso, logger, socket_consola);
			loggear_resultado(logger);
			destroy_pcb(proceso);
			break;
		default:
			log_error(logger, "CÓDIGO DE OPERACIÓN DESCONOCIDO. %d", cod_op);
			break;
	}
	pthread_exit(NULL);
}

t_buffer* recibir_buffer_programa(int socket_consola, t_log* logger) {
	int size;
	t_buffer* buffer = crear_buffer();
	buffer->stream = recibir_buffer(&size, socket_consola);
	buffer->size = size;
	return buffer;
}



t_programa* deserializar_programa(t_buffer* buffer, t_log* logger){
	t_buffer* iterador_buffer = crear_buffer();
	int offset = 0;
	t_programa* programa = crear_programa(list_create());
//	log_info(logger,"Se va a deserializar el programa");

	//Tamaño programa
	memcpy(&(programa->size), &(buffer->size) , sizeof(int));
//	log_info(logger,"Tamaño del programa: %d", programa->size);

	//Tamaño buffer instrucciones
	memcpy(&(iterador_buffer->size), &(buffer->size), sizeof(int));
//	log_info(logger,"Tamaño del buffer de instrucciones: %d", iterador_buffer->size);

	//instrucciones
	iterador_buffer->stream = malloc(iterador_buffer->size);
	memcpy((iterador_buffer->stream), buffer->stream , iterador_buffer->size);
	offset += iterador_buffer->size;

	programa->instrucciones = deserializar_instrucciones(iterador_buffer, logger);
	buffer_destroy(iterador_buffer);

	return programa;
}


t_pcb* crear_proceso(t_programa* programa, t_log* logger,int socket_cpu) {
	t_pcb* pcb = crear_pcb(programa, nuevo_pid());
	squeue_push(colas_planificacion->cola_new, pcb);
	log_info(logger, "Se crea el proceso <%d> en NEW", pcb->pid);
	list_add(procesos_en_kernel,pcb);
	sem_post(&sem_nuevo_proceso);
	return pcb;
}


void respuesta_proceso(t_pcb* proceso,t_log* logger, int socket_consola) {
	sem_wait(&proceso->sem_exit_proceso);
	t_pcb* pcb = squeue_pop(colas_planificacion->cola_exit);
	list_remove_element(procesos_en_kernel, pcb);
	loggear_return_kernel(pcb->pid, pcb->motivo, logger);
	sem_post(&sem_grado_multiprogramacion);
	enviar_handshake(socket_consola, pcb->motivo);

}

void loggear_programa(t_programa* programa,t_log* logger) {
	log_info(logger, "Se recibió un proceso de %d bytes", programa->size);
	log_info(logger, "El proceso cuenta con %d instrucciones", programa->instrucciones->elements_count);
	t_list_iterator* iterador_instrucciones;
	t_list_iterator* iterador_parametros;
	iterador_instrucciones = list_iterator_create(programa->instrucciones);
	for (int i = 0; i < programa->instrucciones->elements_count; i++) {
		t_instruccion* instruccion = (t_instruccion*) list_iterator_next(iterador_instrucciones);
		log_info(logger, "Proceso nro %d: Cód %d - %s",i+1, instruccion->codigo, nombre_de_instruccion(instruccion->codigo));
		int cant_parametros = list_size(instruccion->parametros);
		log_info(logger, "Cantidad de parámetros: %d", cant_parametros);
		iterador_parametros = list_iterator_create(instruccion->parametros);
		for (int j = 0; j < cant_parametros; j++) {
			char* parametro = (char*) list_iterator_next(iterador_parametros);
			log_info(logger, "Parámetro %d: %s", j+1,parametro);
		}
		list_iterator_destroy(iterador_parametros);
	}
	list_iterator_destroy(iterador_instrucciones);
}

void loggear_return_kernel(int pid, int return_kernel, t_log* logger) {
	switch(return_kernel) {
		case SUCCESS:
			log_info(logger, "Finaliza el proceso <%d> - Motivo: SUCCESS", pid);
			break;
		case SEG_FAULT:
			log_info(logger, "Finaliza el proceso <%d> - Motivo: SEG_FAULT", pid);
			break;
		case OUT_OF_MEMORY:
			log_info(logger, "Finaliza el proceso <%d> - Motivo: OUT_OF_MEMORY", pid);
			break;
		default:
			log_error(logger, "Finaliza el proceso <%d> - Motivo: Error innesperado: %d", pid, return_kernel);
			EXIT_FAILURE;
	}
}

int nuevo_pid(void) {
	return pid_contador++;
}


void loggear_resultado(t_log *logger) {
	int control;
	char *log_ejecucion = string_new();
	sem_getvalue(&sem_grado_multiprogramacion, &control);
	//log_info(logger, "Control value -> %d", control);
	//log_info(logger, "Grado Value -> %d", kernel_config->GRADO_MAX_MULTIPROGRAMACION);
	if (control == kernel_config->GRADO_MAX_MULTIPROGRAMACION) {
		int pid;
		int total_procesos = queue_size(colas_planificacion->log_ejecucion->cola);
		log_info(logger, "Procesos ejecutados: %d", total_procesos);
		for (int j = 0; j < total_procesos; j++) {
			//log_info(logger, "true: %d", j);
			pid = (int) squeue_pop(colas_planificacion->log_ejecucion);
			if (pid >= 0) {
				//log_info(logger, "%d", pid);
				if (j != 0) {
					string_append(&log_ejecucion, " -> ");
				}
				char *itoa = string_itoa(pid);
				//log_info(logger, "itoa: %s", itoa);
				string_append(&log_ejecucion, itoa);
			}
		}
		log_info(logger, "RESULTADO -> Orden de ejecución: %s", log_ejecucion);
		queue_clean(colas_planificacion->log_ejecucion->cola);
	}
	free(log_ejecucion);
}



/*void test_timers(t_pcb* pcb) {
	pcb->tiempo_llegada = temporal_create();
	//temporal_resume(pcb->tiempo_llegada);
	int i = 0;
	while (true) {
		if (i > 5) {
			pcb->tiempo_llegada = temporal_create();
			i = 0;
		}
		sleep(1);
		log_info(logger, "Timer iniciado de PID %d: (%d) %ld - %d Time=> %d",
				pcb->pid,
				pcb->tiempo_llegada->status,
				pcb->tiempo_llegada->elapsed_ms,
				pcb->tiempo_llegada->current.tv_sec,
				temporal_gettime(pcb->tiempo_llegada));
		i++;
	}
};*/
