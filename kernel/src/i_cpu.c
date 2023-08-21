#include "../include/i_cpu.h"

t_log* kernel_logger;

void manejar_respuesta_cpu(void* args_hilo){

	t_args_hilo_planificador* args = (t_args_hilo_planificador*) args_hilo;
	kernel_logger = args->log;
	t_config* config = args->config;
	char* algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	op_code cod_op;
	t_contexto_proceso* contexto;

	while(1){
		sem_wait(&proceso_enviado);
		t_pcb* pcb = (t_pcb*) squeue_peek(colas_planificacion->cola_exec);
		cod_op = recibir_operacion(socket_cpu);
		contexto = recibir_contexto(socket_cpu, kernel_logger);
		actualizar_pcb(pcb, contexto);
		procesar_contexto(pcb, cod_op, algoritmo, kernel_logger);
		//loggear_registros(pcb->registros, logger);
	}
}

void actualizar_pcb(t_pcb* pcb, t_contexto_proceso* contexto) {
	pcb->registros = contexto->registros;
	pcb->program_counter = contexto->program_counter;
}

void pasar_a_ready_segun_algoritmo(char* algoritmo,t_pcb* proceso,t_log* logger){
	if(string_equals_ignore_case(algoritmo, "HRRN")) {
		pasar_a_cola_ready_en_orden(proceso, logger, comparador_hrrn);
	} else {
		pasar_a_cola_ready(proceso, logger);
	}
}

void procesar_contexto(t_pcb* pcb, op_code cod_op, char* algoritmo, t_log* logger) {
	char* nombre;
	switch(cod_op) {
		case PROCESO_DESALOJADO_POR_YIELD:
			log_info(logger, "P_CORTO -> Proceso desalojado por Yield");
			pasar_a_cola_ready(pcb, logger);
			sem_post(&cpu_liberada);
			break;
		case PROCESO_FINALIZADO:
			log_info(logger, "P_CORTO -> Proceso desalojado por EXIT");
			solicitar_eliminar_tabla_de_segmento(pcb);
			pasar_a_cola_exit(pcb, logger, SUCCESS);
			sem_post(&cpu_liberada);
			break;
		case PROCESO_BLOQUEADO: //BLOQUEADO POR IO
			log_info(logger, "P_CORTO -> Proceso desalojado por BLOQUEO");
			log_info(logger, "PID: <%d> - Bloqueado por: <IO>", pcb->pid);

			pthread_t thread_bloqueados;
			t_args_hilo_block_io* args = malloc(sizeof(t_args_hilo_block_io));

			int tiempo_bloqueo = recibir_entero(socket_cpu);

			args->algoritmo = algoritmo;
			args->tiempo_bloqueo = tiempo_bloqueo;
			args->pcb = pcb;
			args->logger = logger;
			pcb->estimado_rafaga = pcb->nuevo_estimado;
			pcb->nuevo_estimado = 0;

			pasar_a_cola_blocked(pcb, logger, colas_planificacion->cola_block);
			pthread_create(&thread_bloqueados, NULL, (void *)bloqueo_io, (void*) args);
			pthread_detach(thread_bloqueados);

			sem_post(&cpu_liberada);
			break;
		case PROCESO_DESALOJADO_POR_WAIT:
			nombre = recibir_string(socket_cpu);
			log_info(logger, "P_CORTO -> Proceso desalojado para ejecutar WAIT de %s ", nombre); //TODO modificar log
			procesar_wait_recurso(nombre, pcb, algoritmo, logger);
			break;
		case PROCESO_DESALOJADO_POR_SIGNAL:
			nombre = recibir_string(socket_cpu);
			log_info(logger, "P_CORTO -> Proceso desalojado para ejecutar SIGNAL de %s ", nombre);
			procesar_signal_recurso(nombre, pcb, algoritmo, logger);
			break;
		case PROCESO_DESALOJADO_POR_F_OPEN:
			log_info(logger, "P_CORTO -> Proceso desalojado por F_OPEN");
			procesar_f_open(pcb);
			return;
			break;
		case PROCESO_DESALOJADO_POR_F_CLOSE:
			log_info(logger, "P_CORTO -> Proceso desalojado por F_CLOSE");
			procesar_f_close(pcb);
			ejecutar_proceso(socket_cpu, pcb, logger);
			return;
			break;
		case PROCESO_DESALOJADO_POR_F_SEEK:
			log_info(logger, "P_CORTO -> Proceso desalojado por F_SEEK");
			procesar_f_seek(pcb);
			ejecutar_proceso(socket_cpu, pcb, logger);
			return;
			break;
		case PROCESO_DESALOJADO_POR_F_READ:
			log_info(logger, "P_CORTO -> Proceso desalojado por F_READ");
			pthread_mutex_lock(&puede_compactar);
			procesar_f_read(pcb);
			sem_post(&cpu_liberada);
			return;
			break;
		case PROCESO_DESALOJADO_POR_F_WRITE:
			log_info(logger, "P_CORTO -> Proceso desalojado por F_WRITE");
			pthread_mutex_lock(&puede_compactar);
			procesar_f_write(pcb);
			sem_post(&cpu_liberada);
			return;
			break;
		case PROCESO_DESALOJADO_POR_F_TRUNCATE:
			log_info(logger, "P_CORTO -> Proceso desalojado por F_TRUNCATE");
			procesar_f_truncate(pcb);
			sem_post(&cpu_liberada);
			break;
		case PROCESO_DESALOJADO_POR_CREATE_SEGMENT:
			log_info(logger, "P_CORTO -> Proceso desalojado por CREATE_SEGMENT");
			procesar_create_segment(pcb);
			if (pcb->estado_actual == EXEC) {
				ejecutar_proceso(socket_cpu, pcb, logger);
			} else {
				log_error(logger, "P_CORTO -> Proceso desalojado por OUT_OF_MEMORY");
			}
			break;
		case PROCESO_DESALOJADO_POR_DELETE_SEGMENT:
			log_info(logger, "P_CORTO -> Proceso desalojado por DELETE_SEGMENT");
			procesar_delete_segment(pcb);
			ejecutar_proceso(socket_cpu, pcb, logger);
			break;
		case PROCESO_DESALOJADO_POR_SEG_FAULT:
			log_info(logger, "P_CORTO -> Proceso desalojado por SEG_FAULT");
			solicitar_eliminar_tabla_de_segmento(pcb);
			pasar_a_cola_exit(pcb, logger, SEG_FAULT);
			sem_post(&cpu_liberada);
			return;
			break;
		default:
			log_error(logger, "Error: La respuesta del CPU es innesperada. Cod: %d", cod_op);
			EXIT_FAILURE;
	}

}

void bloqueo_io(void* vArgs){

	t_args_hilo_block_io* args = (t_args_hilo_block_io*) vArgs;
	char* algoritmo = args->algoritmo;
	int tiempo = args->tiempo_bloqueo;
	t_log* logger = args->logger;
	t_pcb* pcb = args->pcb;

	log_info(logger,"PID: <%d> - Ejecuta IO: <%d>", pcb->pid, tiempo);
	sleep(tiempo);
	pasar_a_ready_segun_algoritmo(algoritmo, pcb, logger);
	//TODO REFACTORIZAR A pasar_a_cola_ready
}

void procesar_wait_recurso(char* nombre,t_pcb* pcb,char* algoritmo,t_log* logger) {

	int pos = buscar_recurso(nombre);

	if(pos != -1 ){
		t_recurso* recurso = (t_recurso*)list_get(lista_recursos,pos);
		recurso->instancias--;
		log_info(logger,"PID: <%d> - Wait: <%s> - Instancias: <%d>",pcb->pid,recurso->nombre,recurso->instancias);
		if(recurso->instancias < 0){
			pasar_a_cola_blocked(pcb, logger, recurso->cola_bloqueados);
			log_info(logger,"PID: <%d> - Bloqueado por: <%s>",pcb->pid,nombre);
			sem_post(&cpu_liberada);
		}
		else{
			ejecutar_proceso(socket_cpu, pcb, logger);
		}
	}
	else{
		log_info(logger, "No se encontro recurso %s , pasando PROCESO <%d> a EXIT",nombre,pcb->pid);
		solicitar_eliminar_tabla_de_segmento(pcb);
		pasar_a_cola_exit(pcb, logger, RESOURCE_NOT_FOUND);
		sem_post(&cpu_liberada);
	}

	free(nombre);
}

void procesar_signal_recurso(char* nombre,t_pcb* pcb,char* algoritmo,t_log* logger){

	int pos = buscar_recurso(nombre);

	if(pos != -1){

		t_recurso* recurso = (t_recurso*)list_get(lista_recursos,pos);
		recurso->instancias++;
		log_info(logger,"PID: <%d> - Signal: <%s> - Instancias: <%d>",pcb->pid,nombre,recurso->instancias);

		if(recurso->instancias <= 0){
			pasar_a_ready_segun_algoritmo(algoritmo, squeue_pop(recurso->cola_bloqueados),logger);
			//TODO REFACTORIZAR A pasar_a_cola_ready
		}
		ejecutar_proceso(socket_cpu, pcb, logger);
	}
	else{
		log_info(logger, "No se encontro recurso, pasando a EXIT");
		solicitar_eliminar_tabla_de_segmento(pcb);
		pasar_a_cola_exit(pcb, logger, RESOURCE_NOT_FOUND);
		sem_post(&cpu_liberada);
	}

	free(nombre);
}

void procesar_f_open(t_pcb* pcb) {
	//RECV
	char* nombre_archivo = recibir_string(socket_cpu);
	log_info(kernel_logger,"PID: <%d> - Abrir Archivo: <%s>", pcb->pid, nombre_archivo);
	ejecutar_f_open(pcb, nombre_archivo);
}

void procesar_f_close(t_pcb* pcb) {
	//RECV
	char* nombre_archivo = recibir_string(socket_cpu);
	log_info(kernel_logger,"PID: <%d> - Cerrar Archivo: <%s>", pcb->pid, nombre_archivo);
	ejecutar_f_close(pcb, nombre_archivo);
}

void procesar_f_seek(t_pcb* pcb) {
	//RECV
	char* nombre_archivo = recibir_string(socket_cpu);
	int posicion = recibir_entero(socket_cpu);
	ejectuar_f_seek(pcb->pid, nombre_archivo, posicion);
}

void procesar_f_read(t_pcb* pcb) {
	//RECV
	char* nombre_archivo = recibir_string(socket_cpu);
	int direccion_fisica = recibir_entero(socket_cpu);
	int cantidad_de_bytes = recibir_entero(socket_cpu);

	proceso_fs* proceso = malloc(sizeof(proceso_fs));
	proceso->pcb = pcb;
	proceso->direccion_fisica = direccion_fisica;

	t_archivo_abierto* archivo = obtener_archivo_abierto(nombre_archivo);
	log_info(kernel_logger,"PID: <%d> - Leer Archivo: <%s> - Puntero <%d> - Dirección Memoria <%d> - Tamaño <%d>",
			pcb->pid,
			nombre_archivo,
			archivo->puntero,
			direccion_fisica,
			cantidad_de_bytes);
	squeue_push(colas_planificacion->cola_archivos, proceso);
	sem_post(&request_file_system);
	pasar_a_cola_blocked(pcb, logger, colas_planificacion->cola_block);
}

void procesar_f_write(t_pcb* pcb) {
	//RECV
	char* nombre_archivo = recibir_string(socket_cpu);
	int direccion_fisica = recibir_entero(socket_cpu);
	int cantidad_de_bytes = recibir_entero(socket_cpu);

	proceso_fs* proceso = malloc(sizeof(proceso_fs));
	proceso->pcb = pcb;
	proceso->direccion_fisica = direccion_fisica;

	log_info(kernel_logger,"PID: <%d> - Escrbir Archivo: <%s> - Puntero <PUNTERO> - Dirección Memoria <DIRECCIÓN MEMORIA> - Tamaño <TAMAÑO>",pcb->pid,nombre_archivo);
	squeue_push(colas_planificacion->cola_archivos, proceso);
	sem_post(&request_file_system);
	pasar_a_cola_blocked(pcb, logger, colas_planificacion->cola_block);
}


void procesar_f_truncate(t_pcb* pcb) {
	//RECV
	char* nombre_archivo = recibir_string(socket_cpu);
	int tamanio = recibir_entero(socket_cpu);

	proceso_fs* proceso = malloc(sizeof(proceso_fs));
	proceso->pcb = pcb;

	log_info(kernel_logger,"PID: <%d> - Archivo: <%s> - Tamaño: <%d>", pcb->pid, nombre_archivo, tamanio);
	squeue_push(colas_planificacion->cola_archivos, proceso);
	sem_post(&request_file_system);
	pasar_a_cola_blocked(pcb, logger, colas_planificacion->cola_block);
}

void procesar_create_segment(t_pcb* pcb) {
	//RECV
	int id_segmento = recibir_entero(socket_cpu);
	int tamanio = recibir_entero(socket_cpu);

	//SEND
	pthread_mutex_lock(&mutex_socket_memoria);
	t_paquete* paquete = crear_paquete(MEMORY_CREATE_SEGMENT);
	paquete->buffer = crear_buffer();

	agregar_int_a_paquete(paquete, pcb->pid);
	agregar_int_a_paquete(paquete, id_segmento);
	agregar_int_a_paquete(paquete, tamanio);
	enviar_paquete(paquete, socket_memoria);

	log_info(kernel_logger,"PID: <%d> - Crear Segmento - Id: <%d> - Tamaño: <%d>", pcb->pid, id_segmento, tamanio);

	//RECV
	procesar_respuesta_memoria(pcb);
	pthread_mutex_unlock(&mutex_socket_memoria);
}

void procesar_delete_segment(t_pcb* pcb) {
	//RECV
	int id_segmento = recibir_entero(socket_cpu);

	//SEND
	pthread_mutex_lock(&mutex_socket_memoria);
	t_paquete* paquete = crear_paquete(MEMORY_DELETE_SEGMENT);
	paquete->buffer = crear_buffer();
	agregar_int_a_paquete(paquete, pcb->pid);
	agregar_int_a_paquete(paquete, id_segmento);
	enviar_paquete(paquete, socket_memoria);

	log_info(kernel_logger,"PID: <%d> -  Eliminar Segmento - Id Segmento: <%d>", pcb->pid, id_segmento);

	//RECV
	procesar_respuesta_memoria(pcb);
	pthread_mutex_unlock(&mutex_socket_memoria);
}

void solicitar_eliminar_tabla_de_segmento(t_pcb* pcb) {
	pthread_mutex_lock(&mutex_socket_memoria);
	validar_conexion(socket_memoria);
	log_info(logger, "P_LARGO -> Solicitando Eliminación de Tabla de Segmentos para PID: %d...", pcb->pid);

	//SEND
	t_paquete* paquete = crear_paquete(MEMORY_DELETE_TABLE);
	paquete->buffer = crear_buffer();
	agregar_int_a_paquete(paquete, pcb->pid);
	enviar_paquete(paquete, socket_memoria);
	//RECV
	procesar_respuesta_memoria(pcb);
	pthread_mutex_unlock(&mutex_socket_memoria);
}

void ejecutar_f_open(t_pcb* pcb, char* nombre_archivo) {
	t_archivo_abierto* archivo = obtener_archivo_abierto(nombre_archivo);

	proceso_fs* proceso = malloc(sizeof(proceso_fs));
	proceso->pcb = pcb;

	if (archivo == NULL) {
	// EL ARCHIVO NO ESTA ABIERTO => SE SOLICITA A FILE SYSTEM QUE SE ABRA
	// GOTO EXEC
		log_debug(logger, "FS_THREAD -> El archivo %s no esta abierto. Solicitando apertura...", nombre_archivo);
		squeue_push(colas_planificacion->cola_archivos, proceso);
		sem_post(&request_file_system);
		sem_wait(&f_open_done);
		ejecutar_proceso(socket_cpu, pcb, logger);
	}
	// SI EL ARCHIVO ESTA ABIERTO, SE BLOQUEA EL PROCESO HASTA QUE EL PROCESO ANTERIOR HAGA UN F_CLOSE
	// GOTO BLOCKED
	else {
		log_info(logger, "FS_THREAD -> El archivo %s esta abierto por otro proceso. Bloqueando pid %d...", nombre_archivo, pcb->pid);
		pasar_a_cola_blocked(pcb, logger, colas_planificacion->cola_block);
		squeue_push(archivo->cola_bloqueados, pcb->pid);
		sem_post(&cpu_liberada);
	}
}

void ejectuar_f_seek(int pid, char* nombre_archivo, int posicion_puntero) {
	t_archivo_abierto* archivo = obtener_archivo_abierto(nombre_archivo);
	if (archivo == NULL) {
		log_error(logger, "FS_THREAD -> ERROR: No existe el archivo %s entre los archivos abiertos", nombre_archivo);
		return;
	}

	pthread_mutex_lock(archivo->mutex);
	archivo->puntero = posicion_puntero;
	pthread_mutex_unlock(archivo->mutex);
	log_info(kernel_logger,"PID: <%d> - Actualizar puntero Archivo: <%s> - Puntero <%d>", pid, nombre_archivo, archivo->puntero);
	//log_debug(logger, "FS_THREAD -> Actualizar Puntero Archivo: “PID: <%d> - Actualizar puntero Archivo: <%s> - Puntero <%d>", pid, archivo->nombre, archivo->puntero);
}
