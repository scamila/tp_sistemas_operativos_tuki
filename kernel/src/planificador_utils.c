#include "../include/planificador_utils.h"

t_log* logger;

// SOCKETS
int socket_file_system;
int socket_memoria;

// LARGO PLAZO
sem_t sem_grado_multiprogramacion;
sem_t sem_nuevo_proceso;

// CORTO PLAZO
sem_t cpu_liberada;
sem_t proceso_enviado;

// PLANIFICACIÓN
double alfa = 0.5;
t_colas* colas_planificacion;
t_kernel_config* kernel_config;

// COLAS
sem_t sem_ready_proceso;
sem_t sem_exec_proceso;
sem_t sem_block_proceso;
//sem_t sem_exit_proceso;
pthread_mutex_t mutex_cola_new;
pthread_mutex_t mutex_cola_ready;
pthread_mutex_t mutex_cola_exec;
pthread_mutex_t mutex_cola_exit;

// FILE SYSTEM
sem_t request_file_system;
sem_t f_seek_done;
sem_t f_close_done;
sem_t f_open_done;
t_list* lista_recursos;
t_list* archivos_abiertos;
char** indice_recursos;
int file_id = 0;

// MEMORIA
pthread_mutex_t mutex_socket_memoria;
t_list* procesos_en_kernel; //para usar con compactacion
pthread_mutex_t puede_compactar;

t_squeue* squeue_create(void) {
	t_squeue* squeue = malloc (sizeof(t_squeue));
	squeue->cola = queue_create();
    squeue->mutex = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(squeue->mutex, NULL);
	return squeue;
}

void squeue_destroy(t_squeue* queue) {

	pthread_mutex_destroy(queue->mutex);
	queue_destroy(queue->cola);
	free(queue);
}

void* squeue_pop(t_squeue* queue) {
	pthread_mutex_lock(queue->mutex);
	void* element =  queue_pop(queue->cola);
	pthread_mutex_unlock(queue->mutex);
	return element;
}

void squeue_push(t_squeue* queue, void* element) {
	pthread_mutex_lock(queue->mutex);
	queue_push(queue->cola, element);
	pthread_mutex_unlock(queue->mutex);
}

void* squeue_peek(t_squeue* queue) {
	pthread_mutex_lock(queue->mutex);
	void* element = queue_peek(queue->cola);
	pthread_mutex_unlock(queue->mutex);
	return element;
}

void iniciar_colas_planificacion(void) {

	colas_planificacion = malloc(sizeof(t_colas));
	colas_planificacion->cola_block = squeue_create();
	colas_planificacion->cola_exec = squeue_create();
	colas_planificacion->cola_exit = squeue_create();
	colas_planificacion->cola_new = squeue_create();
	colas_planificacion->cola_ready = squeue_create();
	colas_planificacion->cola_archivos = squeue_create();
	colas_planificacion->log_ejecucion = squeue_create();
}

void destroy_colas_planificacion(void) {

	squeue_destroy(colas_planificacion->cola_block);
	squeue_destroy(colas_planificacion->cola_exec);
	squeue_destroy(colas_planificacion->cola_exit);
	squeue_destroy(colas_planificacion->cola_new);
	squeue_destroy(colas_planificacion->cola_ready);
	squeue_destroy(colas_planificacion->cola_archivos);
	squeue_destroy(colas_planificacion->log_ejecucion);
	free(colas_planificacion);
}

void iniciar_semaforos(int grado_multiprogramacion) {

	sem_init(&sem_grado_multiprogramacion, 0, grado_multiprogramacion);
	sem_init(&sem_nuevo_proceso, 0, 0);
	sem_init(&sem_ready_proceso, 0, 0);
	//sem_init(&sem_exec_proceso, 0, 0);
	sem_init(&sem_block_proceso, 0, 0);
	//sem_init(&sem_exit_proceso, 0, 0);
	sem_init(&cpu_liberada, 0, 1);
	sem_init(&proceso_enviado, 0, 0);
	pthread_mutex_init(&mutex_cola_new, NULL);
	pthread_mutex_init(&mutex_cola_ready, NULL);
	pthread_mutex_init(&mutex_cola_exec, NULL);
	pthread_mutex_init(&mutex_cola_exit, NULL);
	pthread_mutex_init(&mutex_socket_memoria, NULL);
	sem_init(&request_file_system, 0, 0);
	sem_init(&f_seek_done, 0, 0);
	sem_init(&f_close_done, 0, 0);
	sem_init(&f_open_done, 0, 0);
	pthread_mutex_init(&puede_compactar, NULL);
}

void destroy_semaforos(void) {

	sem_destroy(&sem_grado_multiprogramacion);
	sem_destroy(&sem_nuevo_proceso);
	sem_destroy(&sem_ready_proceso);
	//sem_destroy(&sem_exec_proceso);
	sem_destroy(&sem_block_proceso);
	//sem_destroy(&sem_exit_proceso);

	sem_destroy(&cpu_liberada);
	sem_destroy(&proceso_enviado);
	sem_destroy(&request_file_system);
	sem_destroy(&f_seek_done);
	sem_destroy(&f_close_done);
	sem_destroy(&f_open_done);

	pthread_mutex_destroy(&mutex_cola_new);
	pthread_mutex_destroy(&mutex_cola_ready);
	pthread_mutex_destroy(&mutex_cola_exec);
	pthread_mutex_destroy(&mutex_cola_exit);
	pthread_mutex_destroy(&puede_compactar);
}

t_pcb* crear_pcb(t_programa*  programa, int pid_asignado) {
	t_temporal temporal;
	temporal.elapsed_ms = 0;
	temporal.status = -1;

	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->instrucciones = programa->instrucciones;
	pcb->estado_actual = NEW;
	pcb->estimado_rafaga = kernel_config->ESTIMACION_INICIAL;
	pcb->nuevo_estimado = 0;
	pcb->pid = pid_asignado;
	pcb->program_counter = 0;
	pcb->registros = crear_registro();
	pcb->tabla_archivos_abiertos = list_create();
	pcb->tabla_segmento = list_create();
	pcb->tiempo_llegada = &temporal; // malloc(sizeof(t_temporal));
	pcb->tiempo_ejecucion = NULL;
	pcb->motivo = NOT_DEFINED;
	sem_init(&pcb->sem_exit_proceso, 0, 0);
	return pcb;
}

void destroy_pcb(t_pcb* pcb) {
	list_destroy(pcb->instrucciones);
	list_destroy(pcb->tabla_archivos_abiertos);
	list_destroy(pcb->tabla_segmento);
	temporal_destroy(pcb->tiempo_llegada);
	temporal_destroy(pcb->tiempo_ejecucion);
	sem_destroy(&pcb->sem_exit_proceso);
	free(pcb);
}


void pasar_a_cola_ready(t_pcb* pcb, t_log* logger) {

	char* origen = "P_CORTO ";
	switch(pcb->estado_actual){
		case NEW:
			pcb = squeue_pop(colas_planificacion->cola_new);
			origen = "P_LARGO ";
			break;
		case EXEC:
			pcb = squeue_pop(colas_planificacion->cola_exec);
			if (pcb->tiempo_ejecucion != NULL) {
				temporal_stop(pcb->tiempo_ejecucion);
			}
			break;
		case BLOCK:
			//pcb = squeue_pop(colas_planificacion->cola_block);
			list_remove_element(colas_planificacion->cola_block->cola->elements, pcb);
			break;
		case BLOCK_RECURSO:
			break;
		default:
			log_error(logger, "Error, no es un estado válido");
			EXIT_FAILURE;
	}

	char* estado_anterior = estado_string(pcb->estado_actual);
	pcb->estado_actual = READY;
	pcb->tiempo_llegada = temporal_create();
	squeue_push(colas_planificacion->cola_ready,pcb);
	log_info(logger, "%s -> Cambio de Estado: PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", origen, pcb->pid, estado_anterior, estado_string(pcb->estado_actual));
	sem_post(&sem_ready_proceso);
}

void pasar_a_cola_ready_en_orden(t_pcb* pcb_nuevo, t_log* logger, int(*comparador)(t_pcb*, t_pcb*, t_log*)) {
	switch(pcb_nuevo->estado_actual){
		case NEW:
			squeue_pop(colas_planificacion->cola_new);
			break;
		case EXEC:
			squeue_pop(colas_planificacion->cola_exec);
			temporal_stop(pcb_nuevo->tiempo_ejecucion);
			break;
		case BLOCK:
			//squeue_pop(colas_planificacion->cola_block);
			list_remove_element(colas_planificacion->cola_block->cola->elements,pcb_nuevo);
			break;
		case BLOCK_RECURSO:
			break;
		default:
			log_error(logger, "Error, no es un estado válido");
			EXIT_FAILURE;
	}

	char* estado_anterior = estado_string(pcb_nuevo->estado_actual);
	pcb_nuevo->estado_actual = READY;
	int index = -1;
	t_pcb* pcb_lista = NULL;

	if(queue_is_empty(colas_planificacion->cola_ready->cola)) {
		//log_info(logger, "Cola de Ready Vacía. Pusheando");
		squeue_push(colas_planificacion->cola_ready, pcb_nuevo);
	} else {
		//log_info(logger, "Calculando HRRN");
		t_list_iterator* iterador_pcbs = list_iterator_create(colas_planificacion->cola_ready->cola->elements);
		while(list_iterator_has_next(iterador_pcbs)) {
			pcb_lista = (t_pcb*) list_iterator_next(iterador_pcbs);
			//log_info(logger, "iterando index %d", iterador_pcbs->index);
			if (comparador(pcb_nuevo, pcb_lista, logger) == 1) {
				index = iterador_pcbs->index;
				break;
			}
		}
		pcb_nuevo->tiempo_llegada = temporal_create();
		if (index >= 0) {
			list_add_in_index(colas_planificacion->cola_ready->cola->elements, index, pcb_nuevo);
		} else {
			squeue_push(colas_planificacion->cola_ready, pcb_nuevo);
		}

		list_iterator_destroy(iterador_pcbs);
	}
	log_info(logger, "P_CORTO -> Cambio de Estado: PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", pcb_nuevo->pid, estado_anterior, estado_string(pcb_nuevo->estado_actual));
	loggear_cola_ready(logger, "HRRN");
	sem_post(&sem_ready_proceso);
}


double calcular_estimado_proxima_rafaga (t_pcb* pcb, t_log* logger) {
	double real_anterior;
	double estimado_anterior;
	//.current.tv_sec = -1;
	if(pcb->estimado_rafaga <= 0) {
		estimado_anterior = kernel_config->ESTIMACION_INICIAL;
	} else {
		estimado_anterior = (double) pcb->estimado_rafaga;
	}
	//log_info(logger, "Estimado anterior -> %f",estimado_anterior);
	// Si el tiempo es negativo significa que esto se inicializó pero nunca se ejecutó.
	if(pcb->tiempo_ejecucion == NULL) {
		real_anterior = 0.0;
	} else {
		real_anterior = (double) temporal_gettime(pcb->tiempo_ejecucion);
	}
	//log_info(logger, "Real anterior -> %f", real_anterior);
	double nuevo_estimado = alfa * estimado_anterior + (1 - alfa) * real_anterior;
	pcb->nuevo_estimado = nuevo_estimado;
	return nuevo_estimado;
}

void pasar_a_cola_exec(t_pcb* pcb, t_log* logger) {
	if(pcb->estado_actual != READY) {
		log_error(logger, "Error, cola invalida");
		EXIT_FAILURE;
	}
	//log_info(logger,"Candidato PID: %d", pcb->pid);
	pcb = squeue_pop(colas_planificacion->cola_ready);
	char* estado_anterior = estado_string(pcb->estado_actual);
	pcb->estado_actual = EXEC;
	pcb->tiempo_ejecucion = temporal_create();
	temporal_stop(pcb->tiempo_llegada);
	squeue_push(colas_planificacion->cola_exec, pcb);
	squeue_push(colas_planificacion->log_ejecucion, pcb->pid);
	log_info(logger, "P_CORTO -> Cambio de Estado: PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", pcb->pid, estado_anterior, estado_string(pcb->estado_actual));
}

void pasar_a_cola_blocked(t_pcb* pcb, t_log* logger, t_squeue* cola) {
  
	temporal_stop(pcb->tiempo_ejecucion);
	squeue_pop(colas_planificacion->cola_exec);

	char* estado_anterior = estado_string(pcb->estado_actual);
	if(cola == colas_planificacion->cola_block){
		pcb->estado_actual = BLOCK;
	}else{
		pcb->estado_actual = BLOCK_RECURSO;
	}
	//pcb->estado_actual = BLOCK;
	//queue_push(colas_planificacion->cola_block, pcb);
	squeue_push(cola, pcb);
	log_info(logger, "P_CORTO -> Cambio de Estado: PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", pcb->pid, estado_anterior, estado_string(pcb->estado_actual));
	sem_post(&sem_block_proceso);
}

void pasar_a_cola_exit(t_pcb* pcb, t_log* logger, return_code motivo) {
	if(pcb->estado_actual != EXEC){
		log_error(logger, "Error, no es un estado válido");
		EXIT_FAILURE;
	}
	//cerrar_archivos_asociados(pcb);
	squeue_pop(colas_planificacion->cola_exec);
	char* estado_anterior = estado_string(pcb->estado_actual);
	pcb->estado_actual = EXIT;
	pcb->motivo = motivo;
	squeue_push(colas_planificacion->cola_exit, pcb);
	log_info(logger, "P_CORTO -> Cambio de Estado: PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", pcb->pid, estado_anterior, estado_string(pcb->estado_actual));
	sem_post(&pcb->sem_exit_proceso);
}

void cerrar_archivos_asociados(t_pcb* pcb) {
    t_list* archivos_abiertos_pcb = pcb->tabla_archivos_abiertos;

    void cerrar_archivo(void* elemento) {
        t_archivo_abierto* archivo = (t_archivo_abierto*)elemento;
        // TABLA GLOBAL DE ARCFHIVOS ABIERTOS.
        ejecutar_f_close(pcb, archivo->nombre);

    }
    if (list_size(archivos_abiertos_pcb) > 0) {
		list_iterate(archivos_abiertos_pcb, cerrar_archivo);
    }
    // Liberar la memoria utilizada por la lista de archivos abiertos
    list_destroy(archivos_abiertos_pcb);

    // Limpiar la referencia a la lista de archivos abiertos en el PCB
    pcb->tabla_archivos_abiertos = NULL;
}


void ejecutar_proceso(int socket_cpu, t_pcb* pcb, t_log* logger){
	log_info(logger,"P_CORTO -> Ejecutando PID: %d...",pcb->pid);
	t_contexto_proceso* contexto_pcb = malloc(sizeof(t_contexto_proceso));
	contexto_pcb->pid = pcb->pid;
	contexto_pcb->program_counter = pcb->program_counter;
	contexto_pcb->instrucciones = pcb->instrucciones;
	contexto_pcb->registros = pcb->registros;
	contexto_pcb->tabla_segmentos = pcb->tabla_segmento;
//	log_info(logger,"El pcb tiene el PC en %d ",pcb->program_counter);
//	log_info(logger,"El pcb tiene %d instrucciones",list_size(pcb->instrucciones));
//	log_info(logger,"Voy a ejecutar proceso de %d instrucciones", list_size(contexto_pcb->instrucciones));
//	loggear_tabla(pcb);
	enviar_contexto(socket_cpu, contexto_pcb, CONTEXTO_PROCESO, logger);
	sem_post(&proceso_enviado);
	free(contexto_pcb);
}


void loggear_cola_ready(t_log* logger, char* algoritmo) {
	pthread_mutex_lock(colas_planificacion->cola_ready->mutex);
    char* pids = concatenar_pids(colas_planificacion->cola_ready->cola->elements);
    pthread_mutex_unlock(colas_planificacion->cola_ready->mutex);
    log_info(logger, "P_CORTO -> Cola Ready <%s>: [%s]", algoritmo, pids);
    free(pids);
}

char* concatenar_pids(t_list* lista) {

    char* pids = string_new();

    void concatenar_pid(void* elemento) {
        int pid = *((int*)elemento);

        char* pid_str = string_itoa(pid);
        string_append_with_format(&pids, "%s, ", pid_str);
        free(pid_str);
    }

    list_iterate(lista, concatenar_pid);

    if (string_length(pids) > 0) {
        pids = string_substring_until(pids, string_length(pids) - 2);
    }

    return pids;
}


char* estado_string(int cod_op) {
	switch(cod_op) {
		case 0:
			return "NEW";
			break;
		case 1:
			return "READY";
			break;
		case 2:
			return "EXEC";
			break;
		case 3:
			return "BLOCK";
			break;
		case 4:
			return "BLOCK_RECURSO";
		case 5:
			return "EXIT";
			break;
		default:
			printf("Error: Operación de instrucción desconocida\n");
			EXIT_FAILURE;
	}
	return NULL;
}

t_registro crear_registro(void) {

	t_registro registro;
    memset(&registro, 0, sizeof(t_registro));
	return registro;
}

t_temporal* temporal_reset(t_temporal* temporal) {
	if (temporal->status >= 0) {
		temporal_destroy(temporal);
	}
	temporal = temporal_create();
	return temporal;
}

void iniciar_recursos(char** recursos, char** instancias){

	lista_recursos = list_create();
	indice_recursos = recursos;

	for(int i=0;i< string_array_size(recursos);i++){

		t_recurso* recurso = malloc(sizeof(t_recurso));
		recurso->nombre = recursos[i];
		recurso->instancias = atoi(instancias[i]);
		recurso->cola_bloqueados = squeue_create();

		list_add(lista_recursos, recurso);
		//printf("INDICE_RECURSOS[%d] : %s --------> ",i, indice_recursos[i]);
		//printf("RECURSO : %s - INSTANCIAS : %d \n",recurso->nombre,recurso->instancias);
	}
}

int buscar_recurso(char* nombre){

	for(int i=0;i< string_array_size(indice_recursos);i++){
		if(strcmp(nombre,(char*)indice_recursos[i]) == 0){
			return  i;
		}
	}
	return -1;
}

void sincronizar_tabla_segmentos(int socket, t_pcb *pcb) {
	list_destroy(pcb->tabla_segmento);
	pcb->tabla_segmento = recibir_tabla_de_segmentos(socket);
}

void procesar_respuesta_memoria(t_pcb *pcb) {
	//RECV
	validar_conexion(socket_memoria);
	int cod_op = recibir_entero(socket_memoria);
	log_debug(logger, "Recibido memoria op_code: %d", cod_op);
	int pid;
	switch (cod_op) {
		case MEMORY_SEGMENT_TABLE_CREATED: // 67
			pid = recibir_entero(socket_memoria);
			log_info(logger, "P_LARGO -> Asignada Tabla de Segmentos de Memoria para PID: %d", pcb->pid);
			sincronizar_tabla_segmentos(socket_memoria, pcb);
			//loggear_tabla(pcb, "P_LARGO");
			break;
		case MEMORY_SEGMENT_TABLE_DELETED: // 69
			log_info(logger, "P_LARGO -> Eliminada Tabla de Segmentos de Memoria para PID: %d", pcb->pid);
			break;
		case MEMORY_SEGMENT_CREATED: // 65
			pid = recibir_entero(socket_memoria);
			sincronizar_tabla_segmentos(socket_memoria, pcb);
			//loggear_tabla(pcb, "P_CORTO");
			break;
		case MEMORY_SEGMENT_DELETED: // 66
			pid = recibir_entero(socket_memoria);
			sincronizar_tabla_segmentos(socket_memoria, pcb);
//			loggear_tabla(pcb, "P_CORTO");
			break;
		case MEMORY_ERROR_OUT_OF_MEMORY: // 71
			pthread_mutex_unlock(&mutex_socket_memoria); // TODO: PATCH
			solicitar_eliminar_tabla_de_segmento(pcb);
			pasar_a_cola_exit(pcb, logger, OUT_OF_MEMORY);
			sem_post(&cpu_liberada);
			break;
		case MEMORY_NEEDS_TO_COMPACT:
			if (pthread_mutex_trylock(&puede_compactar) == EBUSY) {
				log_info(logger, "Compactación: <Esperando Fin de Operaciones de FS>");
				pthread_mutex_lock(&puede_compactar);
			}
			enviar_entero(socket_memoria, MEMORY_COMPACT);
			log_info(logger, "Compactación: <Se solicitó compactación>");
			procesar_respuesta_memoria(pcb);
			break;
		case MEMORY_COMPACT:
			log_info(logger, "Se finalizó el proceso de compactación");
			sincronizar_tablas_procesos();
			reenviar_create_segment(pcb);
			pthread_mutex_unlock(&puede_compactar);
			break;
		default:
			log_error(logger,"Error: No se pudo crear tabla de segmentos para PID [%d]: Cod %d", pcb->pid, cod_op);
			break;

	}
}

// TODO: REFACTORIZAR A VOID* DE-SERIALIZACION
t_segmento* recibir_segmento(void) {
	t_segmento* segmento = malloc(sizeof(t_segmento));
	segmento->segmento_id = recibir_entero(socket_memoria);
	segmento->inicio= recibir_entero(socket_memoria);
	segmento->tam_segmento = recibir_entero(socket_memoria);
//	log_info(logger, "MEMCHECK -> Segmento ID: %d", segmento->segmento_id);
//	log_info(logger, "MEMCHECK -> Inicio: %d", segmento->inicio);
//	log_info(logger, "MEMCHECK -> Tamaño: %d", segmento->tam_segmento);
	return segmento;
}

t_list* recibir_tabla_segmentos(int socket_memoria) {
	pthread_mutex_lock(&mutex_socket_memoria);
	validar_conexion(socket_memoria);
	t_list* tabla_segmentos = list_create();
	int cant_segmentos = recibir_entero(socket_memoria);
//	log_info(logger, "MEMCHECK -> Cantidad de segmentos: %d", cant_segmentos);
	for (int i = 0; i < cant_segmentos; i++) {
		t_segmento* segmento_aux = recibir_segmento();
		list_add(tabla_segmentos, segmento_aux);
	}
	pthread_mutex_unlock(&mutex_socket_memoria);
	return tabla_segmentos;
}

void sincronizar_tablas_procesos(void) {

	int size = 0;
	void* stream = recibir_buffer(&size, socket_memoria);
	int cantidad_procesos;
	int offset = 0;

	memcpy(&cantidad_procesos, stream, sizeof(int));
	offset += sizeof(int);

	for(int i = 0; i < cantidad_procesos; i++) {

		int pid;
		int size_stream_tabla;

		memcpy(&pid, stream + offset, sizeof(int));
		offset += sizeof(int);
		memcpy(&size_stream_tabla, stream + offset, sizeof(int));
		offset += sizeof(int);
		void* stream_tabla = malloc(size_stream_tabla);
		memcpy(stream_tabla, stream + offset, size_stream_tabla);
		offset += size_stream_tabla;

		bool _buscar_por_pid(void* elem) {
			t_pcb* proceso = (t_pcb*) elem;
			return proceso->pid == pid;
		}

		t_pcb* pcb = list_find(procesos_en_kernel, &_buscar_por_pid);
		list_destroy_and_destroy_elements(pcb->tabla_segmento, &free);
		pcb->tabla_segmento = deserializar_tabla_segmentos(stream_tabla);
		//loggear_segmentos(pcb->tabla_segmento, logger);
	}

	free(stream);
}

void reenviar_create_segment(t_pcb* pcb) {

	t_instruccion* instruccion = list_get(pcb->instrucciones, pcb->program_counter - 1);
	int id_segmento = atoi(list_get(instruccion->parametros, 0));
	int tamanio = atoi(list_get(instruccion->parametros, 1));
	t_paquete* paquete = crear_paquete(MEMORY_CREATE_SEGMENT);
	paquete->buffer = crear_buffer();

	agregar_int_a_paquete(paquete, pcb->pid);
	agregar_int_a_paquete(paquete, id_segmento);
	agregar_int_a_paquete(paquete, tamanio);
	enviar_paquete(paquete, socket_memoria);

	procesar_respuesta_memoria(pcb);
}

void loggear_tablas_archivos(void) {
	log_debug(logger, "Cantidad de Archivos activos: %d", archivos_abiertos->elements_count);
}

t_archivo_abierto* obtener_archivo_abierto(char* nombre_archivo) {
    t_archivo_abierto* archivo_encontrado = NULL;
    void buscar_archivo(t_archivo_abierto* archivo) {
        if (strcmp(archivo->nombre, nombre_archivo) == 0) {
            archivo_encontrado = archivo;
        }
    }
    list_iterate(archivos_abiertos, (void*)buscar_archivo);
    return archivo_encontrado;
}

t_archivo_abierto* crear_archivo_abierto(char* nombre_archivo) {

	t_archivo_abierto* archivo = malloc(sizeof(t_archivo_abierto));
	archivo->cant_aperturas = 0;
	archivo->puntero = 0;
	archivo->file_id = file_id++;
	archivo->mutex = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(archivo->mutex , 0);
	archivo->nombre = string_new();
	archivo->cola_bloqueados = squeue_create();

	return archivo;
}


void archivo_abierto_destroy(t_archivo_abierto* archivo) {

	//free(archivo->nombre); no eliminar, esto elimina 1 parametro de la instruccion
	squeue_destroy(archivo->cola_bloqueados);
    pthread_mutex_destroy(archivo->mutex);
    free(archivo->mutex);
    free(archivo);
}

t_pcb* buscar_pcb_en_lista(int pid, t_list* lista) {
	bool encontrar_pcb(void* elemento) {
		t_pcb* pcb = (t_pcb*)elemento;
		return pcb->pid == pid;
	}
	return (t_pcb*)list_find(lista, encontrar_pcb);
}

void ejecutar_f_close(t_pcb* pcb, char* nombre_archivo) {
	t_archivo_abierto* archivo = obtener_archivo_abierto(nombre_archivo);

	if (archivo == NULL) {
		log_error(logger, "FS_THREAD -> ERROR: No existe el archivo %s entre los archivos abiertos", nombre_archivo);
		return;
	}
	// SI SOLO ESTE PID TIENE ABIERTO EL ARCHIVO
	if (queue_size(archivo->cola_bloqueados->cola) < 1) {
		log_info(logger, "FS_THREAD -> Eliminando entrada en archivo %s para PID %d", nombre_archivo, pcb->pid);
		archivo_abierto_destroy(archivo);
		list_remove_element(archivos_abiertos, archivo); // TABLA GENERAL DE ARCHIVOS ABIERTOS
		loggear_tablas_archivos();
	}
	// SI OTROS PROCESOS ESTAN BLOQUEADOS POR ESTE ARCHIVO -> SE DESBLOQUEA EL PRIMERO
	else
	{
		int pid_a_desbloquear = squeue_pop(archivo->cola_bloqueados);
		log_debug(logger, "FS_THREAD -> Desbloqueando PID %d por F_CLOSE del PID %d", pid_a_desbloquear, pcb->pid);
		// TODO: DESBLOQUEAR OTRO PID
		t_pcb* pcb_a_desbloquear = buscar_pcb_en_lista(pid_a_desbloquear, colas_planificacion->cola_block->cola->elements);
		if (pcb_a_desbloquear == NULL) {
			log_error(logger, "FS_THREAD -> ERROR: no se encontró el pid %d entre los bloqueados para desbloquear", pid_a_desbloquear);
		}
		pasar_a_cola_ready(pcb_a_desbloquear, logger);
	}
}
