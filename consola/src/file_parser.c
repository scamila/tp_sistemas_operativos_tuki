#include "../Include/file_parser.h"

t_programa* parsear_programa(char * archivo, t_log * logger){

	if (access(archivo, F_OK) != 0) {
	    // El archivo no existe
	    log_error(logger,"Error: El archivo %s no existe.\n", archivo);
	    return NULL;
	}

	// r => modo READ
	FILE* file = fopen(archivo, "r");
	if (file == NULL) {
		log_error(logger,"No se puede abrir el archvivo. error:[%d]", errno);
		return NULL;
	}

	t_programa* programa = crear_programa(list_create());
	bool error = parsear(programa, file , logger);
	fclose(file);

	if (error) {
		log_error(logger, "Error de parseo en archivo de pseudocodigo");
		programa_destroy(programa);
		return NULL;
	}
	else {
//		log_info(logger, "retornando programa parseado");
		return programa;
	}

}

bool parsear(t_programa* programa, FILE* file, t_log* logger) {
	bool error = false;
	char* linea = NULL;
	size_t length = 0;

	// GETLINE TOMA LA LINEA DEL TXT HASTA EL PROXIMO SALTO DE LINEA \n
	// Deposita el texto-resultado en el puntero a linea y el largo de la cadena en el puntero a length
	while(getline(&linea, &length, file) != -1) {
		if (length > 0) {
			error = (parsear_instrucciones(linea, programa->instrucciones, logger) != 0);
			if (error) break;
		}
	}
	free(linea);
	return error;
}

void loggear_instrucciones(char **parametros, t_log *logger) {
	char *v;
	int i = 1;
	log_info(logger, "Detectada la funcion %s", parametros[0]);
	while ((v = parametros[i]) != NULL) {
		log_info(logger, "con parámetro nro %d = %s", i, v);
		i++;
	}
	if (i == 1) {
		log_info(logger, "sin parámetros");
	}
}

int parsear_instrucciones(char* linea, t_list* instrucciones, t_log* logger){
	int resultado = EXIT_SUCCESS;
	t_instruccion* instruccion;
	linea = string_replace(linea, "\n", "");
	char** parametros = string_split(linea, " ");
	char* funcion = parametros[0];
	funcion = string_substring_until(funcion, string_length(funcion));
	//loggear_instrucciones(parametros, logger);

	if (strcmp(funcion, "SET") == 0) {
		instruccion = crear_instruccion(ci_SET, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "I/O") == 0) {
		instruccion = crear_instruccion(ci_IO, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "WAIT") == 0) {
		instruccion = crear_instruccion(ci_WAIT, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "SIGNAL") == 0) {
		instruccion = crear_instruccion(ci_SIGNAL, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "YIELD") == 0) {
		instruccion = crear_instruccion(ci_YIELD, true);
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "EXIT") == 0) {
		instruccion = crear_instruccion(ci_EXIT, true);
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "F_OPEN") == 0) {
		instruccion = crear_instruccion(ci_F_OPEN, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "F_READ") == 0) {
		instruccion = crear_instruccion(ci_F_READ, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instruccion->parametros, strdup(parametros[3]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "F_WRITE") == 0) {
		instruccion = crear_instruccion(ci_F_WRITE, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instruccion->parametros, strdup(parametros[3]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "F_TRUNCATE") == 0) {
		instruccion = crear_instruccion(ci_F_TRUNCATE, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "F_SEEK") == 0) {
		instruccion = crear_instruccion(ci_F_SEEK, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "F_CLOSE") == 0) {
		instruccion = crear_instruccion(ci_F_CLOSE, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "CREATE_SEGMENT") == 0) {
		instruccion = crear_instruccion(ci_CREATE_SEGMENT, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "DELETE_SEGMENT") == 0) {
		instruccion = crear_instruccion(ci_DELETE_SEGMENT, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "MOV_IN") == 0) {
		instruccion = crear_instruccion(ci_MOV_IN, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "MOV_OUT") == 0) {
		instruccion = crear_instruccion(ci_MOV_OUT, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instrucciones, instruccion);
	}
	else {
		log_error(logger, "Tipo de instruccion desconocida:[%s]\n", funcion);
		resultado = EXIT_FAILURE;
	}
	liberar_memoria_parseo(parametros, funcion);
	return resultado;
}

t_instruccion* crear_instruccion(t_codigo_instruccion codigo, bool empty) {
	t_instruccion* instruccion = malloc(sizeof(t_instruccion));
	instruccion->codigo = codigo;

	if (empty)
		instruccion->parametros = list_create();
	else
		instruccion->parametros = list_create();

	return instruccion;
}


void liberar_memoria_parseo(char **parametros, char *funcion) {
	char *v;
	int i = 0;
	while ((v = parametros[i]) != NULL) {
		free(v);
		i++;
	}
	free(parametros);
}
