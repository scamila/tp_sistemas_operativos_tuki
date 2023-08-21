#include "../include/i_file_system.h"

void procesar_file_system(void) {

	iniciar_tablas_archivos_abiertos();

	while (true) {
		log_debug(logger, "FS_THREAD -> esperando wait de request de archivo");
		sem_wait(&request_file_system);
	    // Obtener los datos necesarios del PCB para la solicitud
		proceso_fs* p_fs = (proceso_fs*)squeue_pop(colas_planificacion->cola_archivos);
	    log_debug(logger, "FS_THREAD -> El estado del PCB es %s", estado_string(p_fs->pcb->estado_actual));

	    t_instruccion* instruccion = obtener_instruccion(p_fs->pcb);
	   // log_info(logger, "FS_THREAD -> Recibido PID con PC en %d", pcb->program_counter);
	    log_debug(logger, "FS_THREAD -> Llamando a FS por la instrucción -> %d: %s", instruccion->codigo, nombre_de_instruccion(instruccion->codigo));
	    procesar_request_fs(instruccion, p_fs);
	}
}

void procesar_request_fs(t_instruccion *instruccion, proceso_fs *p_fs) {

    char* nombre_archivo = obtener_nombre_archivo(p_fs->pcb);
    log_debug(logger, "FS_THREAD -> Request de pid %d para el archivo %s", p_fs->pcb->pid, nombre_archivo);
    // SEND
	enviar_request_fs(p_fs, instruccion, nombre_archivo);

	// RECV
	recibir_respuesta_fs(nombre_archivo, instruccion, p_fs->pcb);
}


void recibir_respuesta_fs(char *nombre_archivo, t_instruccion *instruccion, t_pcb *pcb) {

	t_archivo_abierto* archivo;
	int pid = pcb->pid;

	// RECV
	int cod_respuesta = recibir_entero(socket_filesystem);
	log_debug(logger, "FS_THREAD -> Recibida respuesta de FILESYSTEM -> %d",	cod_respuesta);
	switch (cod_respuesta) {
	case F_NOT_EXISTS: // RE-SEND
		log_debug(logger,"FS_THREAD -> El Archivo que se intentó abrir no existe. Enviando F_CREATE %s",	nombre_archivo);
		archivo = fs_crear_archivo(nombre_archivo);
		enviar_request_fs(pid, instruccion, nombre_archivo);
		log_info(logger, "FS_THREAD -> Se creó el archivo %s", archivo->nombre);
		recibir_respuesta_fs(nombre_archivo, instruccion, pcb);
		break;
	case F_OPEN_OK:
		archivo = obtener_archivo_abierto(nombre_archivo);
		if (archivo == NULL) {
			log_debug(logger, "FS_THREAD -> Archivo no existente, creando entrada en tabla para %s...", nombre_archivo);
			archivo = crear_archivo_abierto(nombre_archivo);
			archivo->nombre = nombre_archivo;
			log_info(logger, "FS_THREAD -> Creada entrada de archivo para %s...", archivo->nombre);
			//procesar_request_fs(pid,)
		}
		log_info(logger, "FS_THREAD -> Abriendo archivo -> %s", archivo->nombre);
		list_add(archivos_abiertos, archivo);
		archivo->cant_aperturas++;
		list_add(pcb->tabla_archivos_abiertos,archivo);
		//squeue_push(archivo->cola_bloqueados, pid);  no es necesario bloquear al que hace el open. solo a los subsiguientes
		//pasar_a_cola_ready(pcb, logger); el proceso debe seguir ejecutando.
		sem_post(&f_open_done);
		break;
	case F_TRUNCATE_OK:
		archivo = obtener_archivo_abierto(nombre_archivo);
		if (archivo == NULL) {
			log_error(logger, "FS_THREAD -> ERROR: archivo is null");
		}
		log_info(logger, "FS_THREAD -> Archivo Truncado: %s", archivo->nombre);
		pasar_a_cola_ready(pcb, logger);
		break;
	case F_READ_OK:
		pthread_mutex_unlock(&puede_compactar);
		archivo = obtener_archivo_abierto(nombre_archivo);
		if (archivo == NULL) {
			log_error(logger, "FS_THREAD -> ERROR: archivo is null");
		}
		log_info(logger, "FS_THREAD -> F_READ exitoso: %s", archivo->nombre);
		pasar_a_cola_ready(pcb, logger);
		break;
	case F_WRITE_OK:
		pthread_mutex_unlock(&puede_compactar);
		archivo = obtener_archivo_abierto(nombre_archivo);
		if (archivo == NULL) {
			log_error(logger, "FS_THREAD -> ERROR: archivo is null");
		}
		log_info(logger, "FS_THREAD -> F_WRITE exitoso: %s", archivo->nombre);
		pasar_a_cola_ready(pcb, logger);
		break;
	case F_OP_ERROR:
		log_error(logger, "FS_THREAD -> Error al enviar la instrucción -> %d: %s", instruccion->codigo,	nombre_de_instruccion(instruccion->codigo));
		break;
	default:
		break;
	}
}

void enviar_request_fs(proceso_fs* p_fs, t_instruccion* instruccion, char* nombre_archivo) {

	t_archivo_abierto* archivo = obtener_archivo_abierto(nombre_archivo);

	switch (instruccion->codigo) {
		case ci_F_OPEN:
			log_debug(logger, "FS_THREAD -> Enviando Request de ci_F_OPEN para el archivo %s ", nombre_archivo);
			enviar_entero(socket_filesystem, F_OPEN); // f_open ARCHIVO
			enviar_mensaje(nombre_archivo, socket_filesystem);
			break;
		case ci_F_READ:
			//pthread_mutex_lock(&puede_compactar);
			log_debug(logger, "FS_THREAD -> Enviando Request de ci_F_READ para el archivo %s ", nombre_archivo);

			t_paquete* paquete_read = crear_paquete(F_READ);
			paquete_read->buffer = crear_buffer();

			// NOMBRE DE ARCHIVO + SIZE
			agregar_a_paquete(paquete_read, nombre_archivo, string_length(nombre_archivo));
			// DIRECCION FISICA
			agregar_int_a_paquete(paquete_read,p_fs->direccion_fisica);
			// TAMAÑO EN BYTES A ESCRIBIR
			agregar_int_a_paquete(paquete_read,atoi((char*) list_get(instruccion->parametros,2)));
			// PID
			agregar_int_a_paquete(paquete_read, p_fs->pcb->pid);
			// PUNTERO F_SEEK
			agregar_int_a_paquete(paquete_read, archivo->puntero);

			// SEND
			enviar_paquete(paquete_read, socket_filesystem);

			break;
		case ci_F_WRITE:
			//pthread_mutex_lock(&puede_compactar);
			log_debug(logger, "FS_THREAD -> Enviando Request de ci_F_WRITE para el archivo %s ", nombre_archivo);

			t_paquete* paquete = crear_paquete(F_WRITE);
			paquete->buffer = crear_buffer();

			// NOMBRE DE ARCHIVO + SIZE
			agregar_a_paquete(paquete, nombre_archivo, string_length(nombre_archivo));
			// DIRECCION FISICA
			agregar_int_a_paquete(paquete,p_fs->direccion_fisica);
			// TAMAÑO EN BYTES A ESCRIBIR
			agregar_int_a_paquete(paquete,atoi((char*) list_get(instruccion->parametros,2)));
			// PID
			agregar_int_a_paquete(paquete, p_fs->pcb->pid);
			// PUNTERO F_SEEK
			agregar_int_a_paquete(paquete, archivo->puntero);

			enviar_paquete(paquete, socket_filesystem);

			break;
		case ci_F_TRUNCATE:
			log_debug(logger, "FS_THREAD -> Enviando Request de ci_F_TRUNCATE para el archivo %s ", nombre_archivo);
			enviar_entero(socket_filesystem, F_TRUNCATE);
			enviar_mensaje(nombre_archivo, socket_filesystem);
			enviar_mensaje(list_get(instruccion->parametros,1), socket_filesystem);
			break;
		default:
			break;
	}
}

t_instruccion* obtener_instruccion(t_pcb* pcb) {
	return (t_instruccion*)list_get(pcb->instrucciones, pcb->program_counter -1 ); // DEVUELVE LA INSTRUCCION ANTERIOR YA QUE EL CPU AVANZA EL PC SIEMPRE
}

t_archivo_abierto* fs_crear_archivo(char* nombre_archivo) {
	 // Enviar mensaje al módulo de file system para crear el archivo
	int nombre_length = strlen(nombre_archivo) + 1;
	log_info(logger, "FS_THREAD -> Se va a crear el archivo %s (length %d) ", nombre_archivo, nombre_length);
	validar_conexion(socket_filesystem);
	enviar_entero(socket_filesystem, F_CREATE);
	enviar_mensaje(nombre_archivo, socket_filesystem);

	int cod_op = recibir_operacion(socket_filesystem); // F_OP_OK
	if (cod_op != F_OP_OK) {
		log_error(logger, "FS_THREAD -> Error al crear archivo %s. Cod op recibido: %d", nombre_archivo, cod_op);
		return EXIT_FAILURE;
	}
	//int size_buffer = recibir_entero(socket_filesystem);
	//char* path = (char*)recibir_buffer(&size_buffer, socket_filesystem);
	t_archivo_abierto* archivo = crear_archivo_abierto(nombre_archivo);
	archivo->nombre = nombre_archivo;

	return archivo;
}

char* obtener_nombre_archivo(t_pcb* pcb) {
	//log_info(logger, "PID: %d Y PC: %d y SIZE %d", pcb->pid, pcb->program_counter, list_size(pcb->instrucciones));
	t_instruccion* instruccion = (t_instruccion*)list_get(pcb->instrucciones ,pcb->program_counter - 1);
	char* nombre_archivo = (char*) list_get(instruccion->parametros,0);
	return nombre_archivo;
}

void iniciar_tablas_archivos_abiertos(void) {
	archivos_abiertos = list_create();
}

void destroy_tablas_archivos_abiertos(void) {
	list_destroy(archivos_abiertos);
}
