#include "../include/cpu.h"

t_log* cpu_logger;
t_config* config;
int socket_cpu;
int socket_kernel;
int socket_memoria;
t_cpu_config* cpu_config;
t_cpu_config* ip_config;
t_reg registros_cpu;
t_contexto_proceso* proceso;

int main(int argc, char **argv) {

	if(argc < 2){
		printf("Falta path a archivo de configuración.\n");
		return EXIT_FAILURE;
	}

	char* config_path = argv[1];

	cpu_logger = iniciar_logger("cpu.log");
	cargar_config(config_path);

	conexion_a_memoria(cpu_config->ip_memoria,cpu_config->puerto_memoria,cpu_logger);

	correr_servidor();

	terminar();
	printf("CPU FINALIZADA.\n");
	return EXIT_SUCCESS;
}

void terminar(void)
{
	log_info(cpu_logger,"Finalizando CPU...");
	if(cpu_logger != NULL) {
		log_destroy(cpu_logger);
	}

	if(config != NULL) {
		config_destroy(config);
	}

	liberar_conexion(socket_kernel);
	liberar_conexion(socket_memoria);
	liberar_conexion(socket_cpu);
	free(cpu_config);
}

void cargar_config(char* path){
	config = iniciar_config(path);
	cpu_config = malloc(sizeof(t_cpu_config));
	ip_config = iniciar_config("ip_config.config");
	cpu_config->ip_memoria = config_get_string_value(ip_config,"IP_MEMORIA");
	cpu_config->puerto_escucha = config_get_string_value(ip_config,"PUERTO_ESCUCHA");
	cpu_config->puerto_memoria = config_get_string_value(ip_config,"PUERTO_MEMORIA");
	cpu_config->retardo_instruccion = config_get_int_value(config,"RETARDO_INSTRUCCION");
	cpu_config->tam_max_segmento = config_get_int_value(config,"TAM_MAX_SEGMENTO");

}

void conexion_a_memoria(char* ip,char* puerto,t_log* logger){
	log_info(logger,"El módulo CPU se conectará con el MEMORIA con ip %s y puerto: %s  ",ip,puerto);
	socket_memoria = crear_conexion(ip,puerto);
	enviar_handshake(socket_memoria,CPU);
	recibir_operacion(socket_memoria);
	recibir_mensaje(socket_memoria, cpu_logger);

}

void correr_servidor(void){

	socket_cpu = iniciar_servidor(cpu_config->puerto_escucha);
	log_info(cpu_logger, "Iniciada la conexión de servidor de cpu: %d",socket_cpu);

	socket_kernel = esperar_cliente(socket_cpu, cpu_logger);
	int estado_socket = validar_conexion(socket_kernel);

	int modulo = recibir_operacion(socket_kernel);

	switch(modulo){
		case KERNEL:
			log_info(cpu_logger, "Kernel Conectado.");
			while(1){
				int operacion = recibir_operacion(socket_kernel);
				if(operacion == CONTEXTO_PROCESO){ //TODO
					proceso = recibir_contexto(socket_kernel, cpu_logger);

					setear_registros_desde_proceso(proceso);
					ciclo_de_instruccion(proceso,socket_kernel);
					liberar_proceso(proceso);
				}
				else{
					log_info(cpu_logger,"Kernel envió una operacion desconocida");
					break;
				}
			}
			break;
		case -1:
			log_error(cpu_logger, "Se desconectó el cliente.");
			close(socket_kernel);
			exit(EXIT_FAILURE);
		default:
			log_error(cpu_logger, "Codigo de operacion desconocido.");
			return;
			break;

	}
}

void loggear_registros(t_registro* registro) {
    log_info(cpu_logger, "Valores del registro:");
    log_info(cpu_logger, "AX: %s", registro->AX);
    log_info(cpu_logger, "BX: %s", registro->BX);
    log_info(cpu_logger, "CX: %s", registro->CX);
    log_info(cpu_logger, "DX: %s", registro->DX);
    log_info(cpu_logger, "EAX: %s", registro->EAX);
    log_info(cpu_logger, "EBX: %s", registro->EBX);
    log_info(cpu_logger, "ECX: %s", registro->ECX);
    log_info(cpu_logger, "EDX: %s", registro->EDX);
    log_info(cpu_logger, "RAX: %s", registro->RAX);
    log_info(cpu_logger, "RBX: %s", registro->RBX);
    log_info(cpu_logger, "RCX: %s", registro->RCX);
    log_info(cpu_logger, "RDX: %s", registro->RDX);
}

void actualizar_registros_pcb(t_registro* registros) {
    strcpy(registros->AX, registros_cpu.registros_4[0]);
    //log_info(cpu_logger, "Registro AX %s, %s", registros->AX, registros_cpu.registros_4[0]);

    strcpy(registros->BX, registros_cpu.registros_4[1]);
//    log_info(cpu_logger, "Registro BX: %.4s, %s", registros->BX, registros_cpu.registros_4[1]);

    strcpy(registros->CX, registros_cpu.registros_4[2]);
//    log_info(cpu_logger, "Registro CX: %.4s, %s", registros->CX, registros_cpu.registros_4[2]);

    strcpy(registros->DX, registros_cpu.registros_4[3]);
//    log_info(cpu_logger, "Registro DX: %.4s, %s", registros->DX, registros_cpu.registros_4[3]);

    strcpy(registros->EAX, registros_cpu.registros_8[0]);
//    log_info(cpu_logger, "Registro EAX: %.8s, %s", registros->EAX, registros_cpu.registros_8[0]);

    strcpy(registros->EBX, registros_cpu.registros_8[1]);
//    log_info(cpu_logger, "Registro EBX: %.8s, %s", registros->EBX, registros_cpu.registros_8[1]);

    strcpy(registros->ECX, registros_cpu.registros_8[2]);
//    log_info(cpu_logger, "Registro ECX: %.8s, %s", registros->ECX, registros_cpu.registros_8[2]);

    strcpy(registros->EDX, registros_cpu.registros_8[3]);
//    log_info(cpu_logger, "Registro EDX: %.8s, %s", registros->EDX, registros_cpu.registros_8[3]);

    strcpy(registros->RAX, registros_cpu.registros_16[0]);
//    log_info(cpu_logger, "Registro RAX: %.16s, %s", registros->RAX, registros_cpu.registros_16[0]);

    strcpy(registros->RBX, registros_cpu.registros_16[1]);
//    log_info(cpu_logger, "Registro RBX: %.16s, %s", registros->RBX, registros_cpu.registros_16[1]);

    strcpy(registros->RCX, registros_cpu.registros_16[2]);
//    log_info(cpu_logger, "Registro RCX: %.16s, %s", registros->RCX, registros_cpu.registros_16[2]);

    strcpy(registros->RDX, registros_cpu.registros_16[3]);
//    log_info(cpu_logger, "Registro RDX: %.16s, %s", registros->RDX, registros_cpu.registros_16[3]);

   // loggear_registros(registros);
}

void setear_registros_desde_proceso(t_contexto_proceso* proceso){
	t_registro* registros = &proceso->registros;

	strcpy(registros_cpu.registros_4[0], registros->AX );
	strcpy(registros_cpu.registros_4[1], registros->BX );
	strncpy(registros_cpu.registros_4[2], registros->CX, 5);
	strncpy(registros_cpu.registros_4[3], registros->DX,  5);
	strncpy(registros_cpu.registros_8[0], registros->EAX, 9);
	strncpy(registros_cpu.registros_8[1], registros->EBX, 9);
	strncpy(registros_cpu.registros_8[2], registros->ECX, 9);
	strncpy(registros_cpu.registros_8[3], registros->EDX, 9);
	strncpy(registros_cpu.registros_16[0], registros->RAX, 17);
	strncpy(registros_cpu.registros_16[1], registros->RBX, 17);
	strncpy(registros_cpu.registros_16[2], registros->RCX, 17);
	strncpy(registros_cpu.registros_16[3], registros->RDX, 17);
}

void limpiar_registros_cpu(int tam,char registro[][tam]){

	for(int i=0; i < (sizeof(registro)/sizeof(registro[0])) ;i++ ){
		strcpy(&registro[i],"");
	}
}

void liberar_proceso(t_contexto_proceso* proceso){
	limpiar_registros_cpu(5,registros_cpu.registros_4);
	limpiar_registros_cpu(9,registros_cpu.registros_8);
	limpiar_registros_cpu(17,registros_cpu.registros_16);

	list_destroy_and_destroy_elements(proceso->instrucciones,(void*)liberar_parametros_instruccion);
	list_destroy_and_destroy_elements(proceso->tabla_segmentos,&free);
	free(proceso);
}

void liberar_parametros_instruccion(void* instruccion){
	t_instruccion* una_instruccion = (t_instruccion*) instruccion;

	list_destroy_and_destroy_elements(una_instruccion->parametros,(void*)free);

	free(una_instruccion);
}


