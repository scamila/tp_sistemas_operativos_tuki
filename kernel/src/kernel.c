#include "../include/kernel.h"

int socket_filesystem;

int main(int argc, char **argv) {

	logger = iniciar_logger("kernel.log");
	log_info(logger, "MODULO KERNEL");

	/* -- INICIAR CONFIGURACIÓN -- */
	char* config_path = argv[1];

	//char* config_path = "./kernel_hrrn.config";
	if(argc < 1){
		printf("Falta path a archivo de configuración.\n");
		return EXIT_FAILURE;
	}
	t_config* config_kernel = iniciar_config(config_path);
	cargar_config_kernel(config_kernel);


	iniciar_recursos(kernel_config->RECURSOS,kernel_config->INSTANCIAS_RECURSOS);
	iniciar_colas_planificacion();
	iniciar_semaforos(kernel_config->GRADO_MAX_MULTIPROGRAMACION);

	procesos_en_kernel = list_create();
	/* -- CONEXIÓN CON CPU -- */
	socket_cpu = conectar_con_cpu();

    /* -- CONEXIÓN CON MEMORIA -- */
	socket_memoria = conectar_con_memoria();

    /* -- CONEXIÓN CON FILESYSTEM -- */
	socket_filesystem = conectar_con_filesystem();

	t_args_hilo_planificador* args = malloc(sizeof(t_args_hilo_planificador));

	args->log = logger;
	args->config = config_kernel;

	//	PLANIFICACOR DE LARGO PLAZO
	if( pthread_create(&hilo_plp, NULL, planificador_largo_plazo, (void*) args)  != 0) {
		log_error(logger, "Error al inicializar el Hilo Planificador de Largo Plazo");
		exit(EXIT_FAILURE);
	}
	pthread_detach(hilo_plp);

	//	PLANIFICACOR DE CORTO PLAZO
	if( pthread_create(&hilo_pcp, NULL, planificador_corto_plazo, (void*) args)  != 0) {
		log_error(logger, "Error al inicializar el Hilo Planificador de Corto Plazo");
		exit(EXIT_FAILURE);
	}
	pthread_detach(hilo_pcp);

	// MANEJO DE MENSAJES KERNEL-CPU
	if( pthread_create(&hilo_kernel_cpu, NULL, manejar_respuesta_cpu, (void*) args)  != 0) {
		log_error(logger, "Error al inicializar el Hilo KERNEL-CPU");
		exit(EXIT_FAILURE);
	}
	pthread_detach(hilo_kernel_cpu);

	if( pthread_create(&hilo_kernel_fs, NULL, procesar_file_system, (void*) args)  != 0) {
		log_error(logger, "Error al inicializar el Hilo KERNEL-FS");
		exit(EXIT_FAILURE);
	}
	pthread_detach(hilo_kernel_cpu);


	/* -- INICIAR KERNEL COMO SERVIDOR DE CONSOLAS -- */
    socket_kernel = iniciar_servidor(kernel_config->PUERTO_ESCUCHA);
	log_info(logger, "Iniciada la conexión de Kernel como servidor: %d",socket_kernel);


    while(1) {

        log_info(logger, "Esperando conexión de consola...");
        int socket_consola = esperar_cliente(socket_kernel, logger);

//        log_info(logger, "Se conectó una consola con el socket: %d", socket_consola);
        int estado_socket = validar_conexion(socket_consola);
		int modulo = recibir_operacion(socket_consola);
//		log_info(logger, "Recibida op code: %d", modulo);

			switch (modulo) {
				case CONSOLA:

					pthread_t hilo_consola;

					t_args_hilo_cliente* args = malloc(sizeof(t_args_hilo_cliente));

					args->socket = socket_consola;
					args->socket_cpu = socket_cpu;
					args->log = logger;
					enviar_mensaje("Handshake Consola-Kernel", socket_consola);

					//args->mutex = mutex;

					int return_hilo = pthread_create(&hilo_consola, NULL, (void*) procesar_consola, (void*) args);
					if (return_hilo != 0) {
						log_info(logger, "Terminando proceso con exit code de hilo: %d", return_hilo);
						return -1;
					}
					pthread_detach(hilo_consola);

					//free(args);
					break;

				default:
					log_error(logger, "CÓDIGO DE OPERACIÓN DESCONOCIDO.");
					break;
			}
    }

	/* -- FINALIZAR PROGRAMA -- */
	destroy_colas_planificacion();
	destroy_semaforos();
	finalizar_kernel(socket_kernel, logger, config_kernel);
	return EXIT_SUCCESS;
}



void cargar_config_kernel(t_config* config){

	t_config* ip_config = iniciar_config("ip_config.config");
	kernel_config = malloc(sizeof(t_kernel_config));

	kernel_config->IP_MEMORIA = config_get_string_value(ip_config, "IP_MEMORIA");
	kernel_config->PUERTO_MEMORIA = config_get_string_value(ip_config, "PUERTO_MEMORIA");
	kernel_config->IP_FILESYSTEM = config_get_string_value(ip_config, "IP_FILESYSTEM");
	kernel_config->PUERTO_FILESYSTEM = config_get_string_value(ip_config, "PUERTO_FILESYSTEM");
	kernel_config->IP_CPU = config_get_string_value(ip_config, "IP_CPU");
	kernel_config->PUERTO_CPU = config_get_string_value(ip_config, "PUERTO_CPU");
	kernel_config->PUERTO_ESCUCHA = config_get_string_value(ip_config, "PUERTO_ESCUCHA");
	kernel_config->ALGORITMO_PLANIFICACION = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	kernel_config->ESTIMACION_INICIAL = config_get_int_value(config, "ESTIMACION_INICIAL");
	kernel_config->HRRN_ALFA = config_get_int_value(config, "HRRN_ALFA");
	kernel_config->GRADO_MAX_MULTIPROGRAMACION = config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION");
	kernel_config->RECURSOS = config_get_array_value(config, "RECURSOS");
	kernel_config->INSTANCIAS_RECURSOS = config_get_array_value(config, "INSTANCIAS_RECURSOS");

    log_info(logger, "La configuración se cargó en la estructura 'kernel_config' ");

}

int conectar_con_memoria(){

	log_info(logger, "Iniciando la conexión con MEMORIA [IP %s] y [PUERTO:%s]", kernel_config->IP_MEMORIA, kernel_config->PUERTO_MEMORIA);

	socket_memoria = crear_conexion(kernel_config->IP_MEMORIA, kernel_config->PUERTO_MEMORIA);
	enviar_handshake(socket_memoria, KERNEL);
	recibir_operacion(socket_memoria);
	recibir_mensaje(socket_memoria, logger);
	return socket_memoria;
}

int conectar_con_cpu() {

	log_info(logger, "Iniciando la conexión con CPU [IP %s] y [PUERTO:%s]", kernel_config->IP_CPU, kernel_config->PUERTO_CPU);

	socket_cpu = crear_conexion(kernel_config->IP_CPU, kernel_config->PUERTO_CPU);
	enviar_handshake(socket_cpu, KERNEL);

	return socket_cpu;
}

int conectar_con_filesystem(){

	log_info(logger, "Iniciando la conexión con FILESYSTEM [IP %s] y [PUERTO:%s]", kernel_config->IP_FILESYSTEM, kernel_config->PUERTO_FILESYSTEM);

	int socket_filesystem = crear_conexion(kernel_config->IP_FILESYSTEM, kernel_config->PUERTO_FILESYSTEM);
	enviar_handshake(socket_filesystem, KERNEL);
	//recibir_operacion(socket_filesystem);
	//recibir_mensaje(socket_filesystem,logger);
	return socket_filesystem;

}

void finalizar_kernel(int socket_servidor, t_log* logger, t_config* config)
{
	liberar_conexion(socket_cpu);
	liberar_conexion(socket_memoria);
	liberar_conexion(socket_filesystem);
	liberar_conexion(socket_servidor);
	printf("KERNEL FINALIZADO \n");
}
