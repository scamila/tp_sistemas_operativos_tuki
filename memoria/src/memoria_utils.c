#include "../Include/memoria_utils.h"

t_espacio_usuario* espacio_usuario;
t_memoria_config* memoria_config;
t_list* tablas_segmentos;

t_log * logger;
int id = 1;

void iniciar_estructuras(void) {
	espacio_usuario = malloc(sizeof(t_espacio_usuario));
	espacio_usuario->huecos_libres = list_create();
	crear_hueco(0, memoria_config->tam_memoria - 1); // la memoria se inicia con un hueco global
	espacio_usuario->segmentos_activos = list_create();
    log_debug(logger, "Tamaño de memoria: %d", memoria_config->tam_memoria);

    espacio_usuario->espacio_usuario = malloc(memoria_config->tam_memoria);
    memset(espacio_usuario->espacio_usuario,0, memoria_config->tam_memoria);
    if (espacio_usuario->espacio_usuario == NULL) {
        log_error(logger, "No se pudo asignar memoria para el espacio de usuario");
        EXIT_FAILURE;
        return;
    } else {
        int tamaño_asignado = memoria_config->tam_memoria;
        log_debug(logger, "Iniciado espacio de usuario con %d bytes", tamaño_asignado);
    }

    tablas_segmentos = list_create();
}

void destroy_estructuras(void) {
	list_destroy(espacio_usuario->huecos_libres); //TODO AND DESTROY ELEMENTS
	list_destroy(espacio_usuario->segmentos_activos);
	list_destroy_and_destroy_elements(tablas_segmentos, destroy_tabla_segmento);
	//delete_segmento(SEGMENTO_0, SEGMENTO_0);
	free(espacio_usuario->espacio_usuario);
	free(espacio_usuario);
}

t_segmento* crear_segmento(int pid, int tam_segmento, int segmento_id) {
    t_segmento* segmento = malloc(sizeof(t_segmento));

    segmento->descriptor_id = id++;
    segmento->segmento_id = segmento_id; // id autoincremental de sistema (Descriptor)
	segmento->tam_segmento = tam_segmento; // Con la base + el tamaño se calcula la posición final
	log_debug(logger, "Creando segmento con tamaño %d" ,tam_segmento);

	t_hueco* hueco;

	if (pid == SEGMENTO_0) { // SI EL ID ES 0 => SEGMENTO_O, ELSE ES UN PROCESO. PID siempre > 0
		hueco = list_get(espacio_usuario->huecos_libres, 0);
	} else {
		t_tabla_segmento* tabla_segmento = buscar_tabla_segmentos(pid);

		hueco = buscar_hueco(tam_segmento);

		if (hueco == NULL) {
			free(segmento);
			return NULL;
		}
		list_add(tabla_segmento->tabla, segmento);
	}
	segmento->inicio = hueco->inicio;
	log_debug(logger, "Encontrado hueco con piso %d y %d de espacio total", hueco->inicio, tamanio_hueco(hueco));

	log_info(logger, "PID: <%d> - Crear Segmento: <%d> - Base: <%d> - TAMAÑO: <%d>", pid, segmento_id, segmento->inicio, tam_segmento);

	actualizar_hueco(hueco,hueco->inicio + tam_segmento, hueco->fin); // Actualizamos el piso del hueco al nuevo offset.
	list_add(espacio_usuario->segmentos_activos, segmento);

	return segmento;
}

void delete_segmento(int pid, int segmento_id) {

    int descriptor_id = encontrar_descriptor_id(pid, segmento_id);

    bool encontrar_por_id(void* elemento) {
        t_segmento* segmento = (t_segmento*)elemento;
        return segmento->descriptor_id == descriptor_id;
    }
    log_debug(logger, "Eliminando Segmento: DESC_ID %d [PID:%d - SID:%d]...", descriptor_id, pid, segmento_id);

    t_segmento* segmento = list_find(espacio_usuario->segmentos_activos, encontrar_por_id);

    log_info(logger, "PID: <%d> - Eliminar Segmento: <%d> - Base: <%d> - TAMAÑO: <%d>", pid, segmento_id, segmento->inicio, segmento->tam_segmento);

    if (segmento == NULL) {
		log_error(logger, "Error: No se encontró el segmento %d para eliminar", segmento_id);
		return;
	}

    consolidar_huecos_contiguos(segmento->inicio, segmento->tam_segmento);

	if (list_remove_element(espacio_usuario->segmentos_activos, segmento)) { //REMUEVE EL SEGMENTO DE LOS ACTIVOS/OCUPADOS GENERALES
		list_remove_element(encontrar_tabla_segmentos(pid), segmento); //REMUEVE EL SEGMENTO DE LA TABLA PARTICULAR DE ESE PID
		free(segmento); // LIBERA EL SEGMENTO T_SEGMENTO (3 INTS)
	}

    loggear_huecos(espacio_usuario->huecos_libres);
}

t_list* encontrar_tabla_segmentos(int pid) {

	t_tabla_segmento* tabla_encontrada = buscar_tabla_segmentos(pid);
	 if (tabla_encontrada != NULL) {
		 return tabla_encontrada->tabla;
	 } else {
		 log_error(logger, "No se encontró tabla de segmentos para el pid %d", pid);
		 return EXIT_FAILURE;
	 }
}

int encontrar_descriptor_id(int pid, int segmento_id) {

	t_list* tabla_encontrada = encontrar_tabla_segmentos(pid);

	if (tabla_encontrada != NULL) {
		bool encontrar_segmento(void* elemento) {
			t_segmento* segmento = (t_segmento*) elemento;
			return segmento->segmento_id == segmento_id;
		}

		t_segmento* segmento_encontrado = list_find(tabla_encontrada, encontrar_segmento);

		if (segmento_encontrado != NULL) {
			return segmento_encontrado->descriptor_id;
		}
	}

	// En caso de no encontrar el descriptor, puedes devolver un valor predeterminado o manejarlo según tus necesidades
	return 0; // Por ejemplo, devolvemos 0
}

t_hueco* crear_hueco(int inicio, int fin) {
	t_hueco* hueco = malloc(sizeof(t_hueco));
	hueco->inicio = inicio;
	hueco->fin = fin;
	log_debug(logger, "Creado Hueco Libre de memoria de %d Bytes", tamanio_hueco(hueco));

	bool _ordenado_por_base(void* elem1, void* elem2) {
		t_hueco* hueco1 = (t_hueco*) elem1;
		t_hueco* hueco2 = (t_hueco*) elem2;
		return hueco1->inicio < hueco2->inicio;
	}

	list_add_sorted(espacio_usuario->huecos_libres, hueco, &_ordenado_por_base);
	return hueco;
}

void actualizar_hueco(t_hueco* hueco, int nuevo_piso, int nuevo_fin) {
	log_debug(logger, "Actualizando piso de hueco de %d a %d", hueco->inicio, nuevo_piso);

	hueco->inicio = nuevo_piso;
	hueco->fin = nuevo_fin;

	if(hueco->inicio > hueco->fin) {
		log_debug(logger, "El hueco quedó vacío. Eliminado hueco...");
		eliminar_hueco(hueco);
	}
	else{
		log_debug(logger, "Hueco Libre actualizado: [%d-%d]", hueco->inicio, hueco->fin);
	}
}

void eliminar_hueco(t_hueco* hueco) {
	list_remove_element(espacio_usuario->huecos_libres, hueco);
	free(hueco);
}

t_tabla_segmento* crear_tabla_segmento(int pid) {
	t_tabla_segmento* tabla_segmento  = malloc(sizeof(t_tabla_segmento));
	tabla_segmento->tabla = list_create();
	tabla_segmento->pid = pid;

	list_add(tabla_segmento->tabla, list_get(espacio_usuario->segmentos_activos, 0));
	log_debug(logger, "Creada tabla de segmentos para PID %d", pid);
	list_add(tablas_segmentos, tabla_segmento);
	log_debug(logger, "Tablas totales de segmentos: %d", tablas_segmentos->elements_count);
	return tabla_segmento;
}

t_tabla_segmento* buscar_tabla_segmentos(int pid) {
    bool encontrar_por_pid(void* tabla) {
        t_tabla_segmento* tabla_segmento = (t_tabla_segmento*)tabla;
        return tabla_segmento->pid == pid;
    }

    t_tabla_segmento* tabla = (t_tabla_segmento*)list_find(tablas_segmentos, encontrar_por_pid);

    if (tabla == NULL) {
        log_error(logger, "No se encontró la tabla de segmentos para el PID %d", pid);
        return EXIT_FAILURE;
    } else {
    	log_debug(logger, "Encontrada tabla de segmentos para el PID %d con %d segmentos", pid, list_size(tabla->tabla));
    }
    return tabla;
}

void loggear_tablas_segmentos(void) {
    log_debug(logger, "Loggeando tabla de segmentos...");
	int cantidad = tablas_segmentos->elements_count;
	if (cantidad <= 0) {
		log_debug(logger, "No quedan tablas de segmentos en el sistema");
		loggear_huecos(espacio_usuario->huecos_libres);
	} else {
		for (int i = 0; i < cantidad; i++) {
			t_tabla_segmento* tabla_segmentos = list_get(tablas_segmentos, i);
			log_debug(logger, "Tabla de PID: %d", tabla_segmentos->pid);
			int max_tam_segmento = obtener_max_tam_segmento_para_log(tabla_segmentos->tabla);

			for (int j = 0; j < tabla_segmentos->tabla->elements_count; j++) {
				t_segmento* segmento = list_get(tabla_segmentos->tabla, j);
				int barra_size = (segmento->tam_segmento * 20) / max_tam_segmento;  // Ajustar el tamaño de la barra según el máximo tamaño de segmento

				char barra[21];
				memset(barra, '*', barra_size);
				barra[barra_size] = '\0';

				log_debug(logger, "Segmento ID: %d | Inicio: %d | Tamaño: %d | %s", segmento->segmento_id, segmento->inicio, segmento->tam_segmento, barra);
			}
		}
	}
}

int obtener_max_tam_segmento_para_log(t_list* tabla_segmentos) {
    int max_tam = 0;
    for (int i = 0; i < tabla_segmentos->elements_count; i++) {
        t_segmento* segmento = list_get(tabla_segmentos, i);
        if (segmento->tam_segmento > max_tam) {
            max_tam = segmento->tam_segmento;
        }
    }
    return max_tam;
}

void destroy_tabla_segmento(void* elemento) {

	t_tabla_segmento* proceso = (t_tabla_segmento*)elemento;

    liberar_huecos_ocupados(proceso->tabla);
    destroy_elementos_tabla(proceso->tabla);
    log_debug(logger, "Eliminada tabla de segmentos de PID %d", proceso->pid);
    list_remove_element(tablas_segmentos, proceso);
    log_debug(logger, "Eliminada entrada en tablas de segmentos para PID %d",proceso->pid);
    free(proceso);
    loggear_tablas_segmentos();
}

void destroy_elementos_tabla(t_list* tabla) {
	list_remove(tabla, 0);

	void _aux(void* elem) {
		t_segmento* segmento = (t_segmento*) elem;
		list_remove_element(espacio_usuario->segmentos_activos, segmento);
		free(segmento);
	}
	list_destroy_and_destroy_elements(tabla, &_aux);
}

void liberar_huecos_ocupados(t_list* tabla) {

	t_list_iterator* segmento_iterator = list_iterator_create(tabla);
	while(list_iterator_has_next(segmento_iterator)) {
		t_segmento* segmento_aux = (t_segmento*) list_iterator_next(segmento_iterator);
		if(segmento_aux->segmento_id != SEGMENTO_0) {
			consolidar_huecos_contiguos(segmento_aux->inicio, segmento_aux->tam_segmento);
		}
	}
	list_iterator_destroy(segmento_iterator);
}

t_memoria_config* leer_config(char *path) {
    t_config *config = iniciar_config(path);
    t_config* ip_config = iniciar_config("ip_config.config");
    t_memoria_config* tmp = malloc(sizeof(t_memoria_config));

    tmp->puerto_escucha = config_get_string_value(ip_config, "PUERTO_ESCUCHA");
    log_info(logger, "Puerto de escucha: %s", tmp->puerto_escucha);

    tmp->tam_memoria = config_get_int_value(config,"TAM_MEMORIA");
//    log_info(logger, "Tamaño de memoria: %d", tmp->tam_memoria);

    tmp->tam_segmento_0 = config_get_int_value(config,"TAM_SEGMENTO_0");
//    log_info(logger, "Tamaño de segmento 0: %d", tmp->tam_segmento_0);

    tmp->cant_segmentos = config_get_int_value(config,"CANT_SEGMENTOS");
//    log_info(logger, "Cantidad de segmentos: %d", tmp->cant_segmentos);

    tmp->retardo_memoria = config_get_int_value(config,"RETARDO_MEMORIA");
//    log_info(logger, "Retardo de memoria: %d", tmp->retardo_memoria);

    tmp->retardo_compactacion = config_get_int_value(config,"RETARDO_COMPACTACION");
//    log_info(logger, "Retardo de compactación: %d", tmp->retardo_compactacion);

    tmp->algoritmo_asignacion = config_get_string_value(config,"ALGORITMO_ASIGNACION");
//    log_info(logger, "Algoritmo de asignación: %s", tmp->algoritmo_asignacion);

    //config_destroy(config);
    return tmp;
}

void correr_servidor(t_log *logger, char *puerto) {

	int server_fd = iniciar_servidor(puerto);

	log_info(logger, "Iniciada la conexión de servidor de memoria: %d",server_fd);

	while(escuchar_clientes(server_fd,logger));

	liberar_conexion(server_fd);

}

int aceptar_cliente(int socket_servidor) {
	struct sockaddr_in dir_cliente;
	int tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente,(void*) &tam_direccion);

	return socket_cliente;
}

void consolidar_huecos_contiguos(int inicio_nuevo_espacio, int tamanio_nuevo_espacio) {

	t_hueco* hueco_derecho = buscar_hueco_por_posicion_inicial(inicio_nuevo_espacio + tamanio_nuevo_espacio);
	t_hueco* hueco_izquierdo = buscar_hueco_por_posicion_limite(inicio_nuevo_espacio - 1);

	if(hueco_izquierdo != NULL){
		if(hueco_derecho != NULL){
			actualizar_hueco(hueco_izquierdo, hueco_izquierdo->inicio, hueco_derecho->fin);
			eliminar_hueco(hueco_derecho);
		}
		else {
			actualizar_hueco(hueco_izquierdo, hueco_izquierdo->inicio, hueco_izquierdo->fin + tamanio_nuevo_espacio);
		}
	}
	else if(hueco_derecho != NULL){
		actualizar_hueco(hueco_derecho, inicio_nuevo_espacio, hueco_derecho->fin);
	}
	else{
		crear_hueco(inicio_nuevo_espacio, inicio_nuevo_espacio + tamanio_nuevo_espacio - 1);
	}
}

t_hueco* buscar_hueco(int tamanio) {

	char* algoritmo = memoria_config->algoritmo_asignacion;
	t_hueco* hueco;

	if(string_equals_ignore_case(algoritmo, "BEST")){
		hueco = buscar_hueco_por_best_fit(tamanio);
	}
	else if(string_equals_ignore_case(algoritmo, "FIRST")){
		hueco = buscar_hueco_por_first_fit(tamanio);
	}
	else{
		hueco = buscar_hueco_por_worst_fit(tamanio);
	}
	return hueco;
}

t_list* filtrar_huecos_libres_por_tamanio(int tamanio) {

	bool _func_aux(void* elemento){
		t_hueco* hueco = (t_hueco*) elemento;
		return tamanio_hueco(hueco) >= tamanio;
	}
	return list_filter(espacio_usuario->huecos_libres, &_func_aux);
}

t_hueco* buscar_hueco_por_best_fit(int tamanio){

	t_list* huecos_candidatos = filtrar_huecos_libres_por_tamanio(tamanio);

	if (list_size(huecos_candidatos) <= 0) {
		return NULL;
	}
	void* _fun_aux_2(void* elem1, void* elem2){
		t_hueco* hueco1 = (t_hueco*) elem1;
		t_hueco* hueco2 = (t_hueco*) elem2;

		if(tamanio_hueco(hueco1) < tamanio_hueco(hueco2) ){
			return hueco1;
		}
		return hueco2;
	}
	return (t_hueco*) list_get_minimum(huecos_candidatos, &_fun_aux_2);
}

t_hueco* buscar_hueco_por_first_fit(int tamanio) {
	t_list* huecos_candidatos = filtrar_huecos_libres_por_tamanio(tamanio);
	if (list_size(huecos_candidatos) <= 0) {
		return NULL;
	}
	return (t_hueco*) list_get(huecos_candidatos, 0);
}

t_hueco* buscar_hueco_por_worst_fit(int tamanio) {

	t_list* huecos_candidatos = filtrar_huecos_libres_por_tamanio(tamanio);
	if (list_size(huecos_candidatos) <= 0) {
		return NULL;
	}
	void* _fun_aux_2(void* elem1, void* elem2){
		t_hueco* hueco1 = (t_hueco*) elem1;
		t_hueco* hueco2 = (t_hueco*) elem2;

		if(tamanio_hueco(hueco1) > tamanio_hueco(hueco2) ){//TODO CUAL DEVOLVER SI SON IGUALES?
			return hueco1;
		}
		return hueco2;
	}
	return (t_hueco*) list_get_maximum(huecos_candidatos, &_fun_aux_2);
}

int tamanio_hueco(t_hueco* hueco) {
	return hueco->fin - hueco->inicio + 1 ;
}

void loggear_huecos(t_list* huecos) {
	log_debug(logger, "----------HUECOS----------");
	log_debug(logger, "	INICIO	FIN");
	void _log(void* elem){
		t_hueco* hueco  = (t_hueco*) elem;

		log_debug(logger, "	%d	%d", hueco->inicio, hueco->fin);
	}
	list_iterate(huecos, &_log);
}

char* leer_direccion(int direccion, int tamanio) {

	char* valor = malloc(tamanio);
	memcpy(valor, espacio_usuario->espacio_usuario + direccion, tamanio);
	usleep(memoria_config->retardo_memoria * 1000);
	return valor;
}

void escribir_en_direccion(int direccion, int tamanio, char* valor_a_escribir, int socket_cliente) {
	memcpy(espacio_usuario->espacio_usuario + direccion, valor_a_escribir, tamanio);
	enviar_mensaje("OK", socket_cliente);
	free(valor_a_escribir);
	usleep(memoria_config->retardo_memoria * 1000);
}

// COMPACTACION
int memoria_disponible(void) {

	int _total(int elem1, void* elem2){
		return elem1 + tamanio_hueco((t_hueco*) elem2);
	}

	int total = list_fold(espacio_usuario->huecos_libres, 0, &_total);

	log_debug(logger, "Espacio disponible en memoria : %d ", total);
	return total;
}

void compactar_memoria(void) {

	log_debug(logger, "SEGMENTOS ACTIVOS - antes de compactar");
	loggear_segmentos(espacio_usuario->segmentos_activos, logger);
	loggear_huecos(espacio_usuario->huecos_libres);

	for(int i=0; i < list_size(espacio_usuario->segmentos_activos); i++){
		t_segmento* segmento_a_mover = list_get(espacio_usuario->segmentos_activos, i);
		t_hueco* hueco_a_usar = buscar_hueco_por_posicion_limite(segmento_a_mover->inicio - 1); //por cada segmento activo busca si tiene un hueco en la posicion anterior

		if(hueco_a_usar == NULL){
			continue;
		}
		int tam_hueco = tamanio_hueco(hueco_a_usar);

		memmove(espacio_usuario->espacio_usuario + hueco_a_usar->inicio, espacio_usuario->espacio_usuario + segmento_a_mover->inicio, segmento_a_mover->tam_segmento);
		segmento_a_mover->inicio -= tam_hueco ;
		eliminar_hueco(hueco_a_usar);
		consolidar_huecos_contiguos(segmento_a_mover->inicio + segmento_a_mover->tam_segmento, tam_hueco);
	}
	log_debug(logger, "SEGMENTOS ACTIVOS - despues de compactar");
	loggear_segmentos(espacio_usuario->segmentos_activos, logger);
	loggear_huecos(espacio_usuario->huecos_libres);
	sleep(memoria_config->retardo_compactacion / 1000);
}

void resultado_compactacion(void) {

	void _loggear_tabla(void* elem) {
		t_tabla_segmento* proceso = (t_tabla_segmento*) elem;

		void _log_segmento(void* elem) {
			t_segmento* segmento = (t_segmento*) elem;
			log_info(logger, "PID: <%d> - Segmento: <%d> - Base: <%d> - Tamaño <%d>", proceso->pid, segmento->segmento_id, segmento->inicio, segmento->tam_segmento);
		}
		list_iterate(proceso->tabla, &_log_segmento);
	}
	list_iterate(tablas_segmentos, &_loggear_tabla);
}

t_hueco* buscar_hueco_por_posicion_limite(int posicion) {

	bool _comparar_fin(void* elem) {
		t_hueco* hueco = (t_hueco*) elem;
		return hueco->fin == posicion;
	}
	return list_find(espacio_usuario->huecos_libres, &_comparar_fin);
}

t_hueco* buscar_hueco_por_posicion_inicial(int posicion) {

	bool _comparar_inicio(void* elem) {
		t_hueco* hueco = (t_hueco*) elem;
		return hueco->inicio == posicion;
	}
	return list_find(espacio_usuario->huecos_libres, &_comparar_inicio);
}

