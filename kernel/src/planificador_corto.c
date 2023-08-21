#include "../include/planificador_corto.h"

extern int socket_cpu;

int planificador_corto_plazo(void* args_hilo) {
	t_args_hilo_planificador* args = (t_args_hilo_planificador*) args_hilo;
	logger = args->log;
	log_info(logger, "P_CORTO -> Inicializado Hilo Planificador de Corto Plazo");
	t_config* config = args->config;
    char* algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    log_info(logger, "P_CORTO -> Algoritmo: %s", algoritmo);
	while(1) {
		sem_wait(&cpu_liberada);
		sem_wait(&sem_ready_proceso);
		t_pcb* pcb = planificar(algoritmo, logger);
		//loggear_registros(pcb->registros, logger);
		ejecutar_proceso(socket_cpu, pcb, logger);
	}
	return 1;
}

t_pcb* planificar(char* algoritmo, t_log* logger) {
	if (string_equals_ignore_case(algoritmo, "HRRN")) {
		//log_info(logger, "planificando con %s", algoritmo);
		t_pcb* pcb = proximo_proceso_hrrn(logger);
		//loggear_cola_ready(logger, algoritmo);
		pasar_a_cola_exec(pcb, logger);
		return pcb;
	} else if (string_equals_ignore_case(algoritmo, "FIFO")){
		loggear_cola_ready(logger, kernel_config->ALGORITMO_PLANIFICACION);
		t_pcb* pcb = (t_pcb*)squeue_peek(colas_planificacion->cola_ready);
		pasar_a_cola_exec(pcb, logger);
		//log_info(logger,"PID:%d ",pcb->pid);
		return pcb;
	} else {
		log_error(logger, "Error: No existe el algoritmo: %s", algoritmo);
		EXIT_FAILURE;
	}
}

t_pcb* proximo_proceso_hrrn(t_log* logger) {
	pthread_mutex_lock(colas_planificacion->cola_ready->mutex);
	t_list* lista_ready = colas_planificacion->cola_ready->cola->elements;
	pthread_mutex_unlock(colas_planificacion->cola_ready->mutex);
	loggear_cola_ready(logger, kernel_config->ALGORITMO_PLANIFICACION);
	if (list_size(lista_ready) > 1) {
		list_sort(lista_ready, comparador_hrrn);
	}
	t_pcb* pcb = squeue_peek(colas_planificacion->cola_ready);
	//log_info(logger,"Candidato PID: %d", pcb->pid);
	loggear_cola_ready(logger, kernel_config->ALGORITMO_PLANIFICACION);
	return pcb;
}

bool comparador_hrrn(void* pcb1, void* pcb2) {
	//log_info(logger,"||||||||| COMPARADOR |||||||||");
	t_pcb* pcb_nuevo = (t_pcb*) pcb1;
	t_pcb* pcb_lista = (t_pcb*) pcb2;

	double S_pcb_nuevo = calcular_estimado_proxima_rafaga(pcb_nuevo, logger);
	double S_pcb_lista =  calcular_estimado_proxima_rafaga(pcb_lista, logger);

	//log_info(logger, "Estimado anterior: %d", pcb_nuevo->estimado_rafaga);
	//log_info(logger, "Estimado nuevo: %d", pcb_nuevo->nuevo_estimado);
	int64_t W_pcb_nuevo =  temporal_gettime(pcb_nuevo->tiempo_llegada);
	int64_t W_pcb_lista =  temporal_gettime(pcb_lista->tiempo_llegada);

	double ratio_pcb_nuevo = (S_pcb_nuevo + W_pcb_nuevo) / (double)S_pcb_nuevo;
	double ratio_pcb_lista = (S_pcb_lista + W_pcb_lista) / (double)S_pcb_lista;

	if(ratio_pcb_nuevo >= ratio_pcb_lista) {
//		log_info(logger, "HRRN -> PID %d - [S: %f] - [W: %d] - [RR: %f] > PID %d: - [S: %f] - [W: %d] - [RR:%f]" ,
//					pcb_nuevo->pid, S_pcb_nuevo, W_pcb_nuevo, ratio_pcb_nuevo,
//					pcb_lista->pid, S_pcb_lista, W_pcb_lista, ratio_pcb_lista);
		return true;
	} else {
//		log_info(logger, "HRRN -> PID %d - [S: %f] - [W: %d] - [RR: %f] < PID %d: - [S: %f] - [W: %d] - [RR:%f]"  ,
//					pcb_nuevo->pid, S_pcb_nuevo, W_pcb_nuevo, ratio_pcb_nuevo,
//					pcb_lista->pid, S_pcb_lista, W_pcb_lista, ratio_pcb_lista);
		return false;
	}
}

void loggear_registros(t_registro registro, t_log* logger) {
    log_info(logger, "Valores del registro:");
    log_info(logger, "AX: %.*s", 4, registro.AX);
    log_info(logger, "BX: %.*s", 4, registro.BX);
    log_info(logger, "CX: %.*s", 4, registro.CX);
    log_info(logger, "DX: %.*s", 4, registro.DX);
    log_info(logger, "EAX: %.*s", 8, registro.EAX);
    log_info(logger, "EBX: %.*s", 8, registro.EBX);
    log_info(logger, "ECX: %.*s", 8, registro.ECX);
    log_info(logger, "EDX: %.*s", 8, registro.EDX);
    log_info(logger, "RAX: %.*s", 16, registro.RAX);
    log_info(logger, "RBX: %.*s", 16, registro.RBX);
    log_info(logger, "RCX: %.*s", 16, registro.RCX);
    log_info(logger, "RDX: %.*s", 16, registro.RDX);
}


