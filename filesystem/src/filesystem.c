#include "../Include/filesystem.h"

int main(int argc, char **argv) {

	logger = iniciar_logger("file_system.log");
	if (logger == NULL){
		log_error(logger, "NO se inicializó el log.");
		exit(1);
	}

	log_info(logger, "MODULO FILE SYSTEM");

	config = iniciar_config(argv[1]);
	ip_config = iniciar_config("ip_config.config");
    if (config == NULL){
		log_error(logger, "NO se inicializó la configuración del FS");
		exit(2);
	}

    cargar_config_fs(config);
    iniciar_fs();
    conectar_con_memoria();
    correr_servidor();

    return EXIT_SUCCESS;
}

void cargar_config_fs(t_config* config) {
    fs_config = malloc(sizeof(t_fs_config));
    fs_config->IP_MEMORIA = config_get_string_value(ip_config, "IP_MEMORIA");
    fs_config->PUERTO_MEMORIA = config_get_string_value(ip_config, "PUERTO_MEMORIA");
    fs_config->PUERTO_ESCUCHA = config_get_string_value(ip_config, "PUERTO_ESCUCHA");
    fs_config->PATH_SUPERBLOQUE = config_get_string_value(config, "PATH_SUPERBLOQUE");
    fs_config->PATH_BITMAP = config_get_string_value(config, "PATH_BITMAP");
    fs_config->PATH_BLOQUES = config_get_string_value(config, "PATH_BLOQUES");
    fs_config->PATH_FCB = config_get_string_value(config, "PATH_FCB");
    fs_config->RETARDO_ACCESO_BLOQUE = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");

    log_debug(logger, "La configuración se cargó en la estructura fs_config");
}

void iniciar_fs() {
	if (existe_fs() == 0) {
        log_info(logger, "File System encontrado, recuperando...");
        iniciar_superbloque();
        iniciar_bitmap();
        iniciar_bloques();
        iniciar_FCBs();
    } else {
    	log_info(logger, "File System NO encontrado, generando...");
        crear_directorio(fs_config->PATH_FCB);
        iniciar_fs();
    }
}

int existe_fs() {
	return access(fs_config->PATH_FCB, F_OK);
}

void iniciar_superbloque(){
	config = iniciar_config(fs_config->PATH_SUPERBLOQUE);
	superbloque = malloc(sizeof(Superbloque));
    superbloque->BLOCK_COUNT = config_get_int_value(config, "BLOCK_COUNT");
    superbloque->BLOCK_SIZE = config_get_int_value(config, "BLOCK_SIZE");
    log_debug(logger, "ARCHIVO %s LEIDO", fs_config->PATH_SUPERBLOQUE);
	config_destroy(config);
}

void iniciar_bitmap() {

	char* rutaBitmap = fs_config->PATH_BITMAP;
	int fd = open(rutaBitmap, O_CREAT | O_RDWR, 0777);
    int cantidadBloques; // 65536 -> tengo 65536 bloques

    if ((superbloque->BLOCK_COUNT % 8) == 0) {
        cantidadBloques = superbloque->BLOCK_COUNT / 8;
    } else {
        cantidadBloques = (superbloque->BLOCK_COUNT / 8) + 1;
    }

    ftruncate(fd, cantidadBloques);
    mapBitmap = mmap(NULL, cantidadBloques, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapBitmap == MAP_FAILED) {
    	close(fd);
    	exit(1);
	}
    msync(mapBitmap, sizeof(uint32_t), MS_SYNC); //sincroniza el bitmap cargado en memoria con el archivo en disco
    bitmap = bitarray_create_with_mode(mapBitmap, cantidadBloques, LSB_FIRST);
    log_debug(logger, "El tamaño del bitmap creado es de %zu bits.", bitarray_get_max_bit(bitmap));

    close(fd);
    log_debug(logger, "ARCHIVO %s LEIDO", rutaBitmap);
    //imprimir_bitmap(bitmap);
}

void imprimir_bitmap(t_bitarray* bitmap){
	printf("\n\nimprimiendo bitmap: \n[");
		for (int i = 0; i < superbloque->BLOCK_COUNT; i++) {
	        printf("%d ", bitarray_test_bit(bitmap, i));
	    }
		printf("]\n\n");
}

void iniciar_bloques(void) {

	char* rutaBloques = fs_config->PATH_BLOQUES;
    FILE* archivo_bloques = fopen(rutaBloques, "rb+");
    if (archivo_bloques == NULL) {
        archivo_bloques = fopen(rutaBloques, "wb+");
        if (archivo_bloques == NULL) {
            log_error(logger, "No se pudo abrir o crear el archivo bloques");
            exit(1);
        }
    }

    fseek(archivo_bloques, 0L, SEEK_END);
    int size_archivo = ftell(archivo_bloques);
    log_debug(logger, "Posicion del archivo: %d", size_archivo);

    if (size_archivo > 0) {
        size_t size = superbloque->BLOCK_COUNT * superbloque->BLOCK_SIZE;
        log_debug(logger, "Size de %zu", size);

        int fd = fileno(archivo_bloques);
        bloques = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (bloques == MAP_FAILED) {
            log_error(logger, "Error al mapear el archivo de bloques");
        } else {
        	log_debug(logger, "Lluege %s", (char*)bloques);
            msync(bloques, sizeof(uint32_t), MS_SYNC); //sincroniza el bitmap cargado en memoria con el archivo en disco
        }
    } else {
        bloques = NULL;
    }

    fclose(archivo_bloques);
    log_debug(logger, "ARCHIVO %s LEIDO", rutaBloques);
}

void iniciar_FCBs(){

	lista_fcb = list_create();
	DIR* dir = opendir(fs_config->PATH_FCB);
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
    	if (entry->d_type == DT_REG){
			char* path = string_new();
			string_append(&path, fs_config->PATH_FCB);
			string_append(&path, "/");
			string_append(&path, entry->d_name);

			t_config* fcb_config = config_create(path);

			char* nombre = config_get_string_value(fcb_config, "NOMBRE_ARCHIVO");
			uint32_t tamanio = config_get_int_value(fcb_config, "TAMANIO_ARCHIVO");
			uint32_t puntero_directo = config_get_int_value(fcb_config, "PUNTERO_DIRECTO");
			uint32_t puntero_indirecto = config_get_int_value(fcb_config, "PUNTERO_INDIRECTO");

			t_fcb* fcb = leer_fcb(nombre, tamanio, puntero_directo, puntero_indirecto);
			list_add(lista_fcb, fcb);
			config_destroy(fcb_config);
			free(path);
    	}
	}

	closedir(dir);
}

t_fcb* leer_fcb(char* nombre, uint32_t tamanio, uint32_t puntero_directo, uint32_t puntero_indirecto){
	t_fcb* fcb;
	fcb = malloc(sizeof(t_fcb));
	fcb->NOMBRE_ARCHIVO = string_new();
	string_append(&fcb->NOMBRE_ARCHIVO, nombre);
	fcb->TAMANIO_ARCHIVO = tamanio;
	fcb->PUNTERO_DIRECTO = puntero_directo;
	fcb->PUNTERO_INDIRECTO = puntero_indirecto;
	return fcb;
}

void crear_directorio(char* ruta) {
    char* sep = strrchr(ruta, '/');
    if (sep != NULL) {
        *sep = '\0';
        crear_directorio(ruta);
        *sep = '/';
    }
    mkdir(ruta, S_IRWXU | S_IRWXG | S_IRWXO);
}

void conectar_con_memoria() {
	log_info(logger, "Iniciando la conexión con MEMORIA [IP %s] y [PUERTO:%s]", fs_config->IP_MEMORIA, fs_config->PUERTO_MEMORIA);
    socket_memoria = crear_conexion(fs_config->IP_MEMORIA, fs_config->PUERTO_MEMORIA);
    enviar_handshake(socket_memoria, FILESYSTEM);
    recibir_entero(socket_memoria);
    recibir_mensaje(socket_memoria, logger);
}

void correr_servidor(void) {
	socket_fs = iniciar_servidor(fs_config->PUERTO_ESCUCHA);
	log_info(logger, "Iniciada la conexión servidor de FS: %d", socket_fs);
    socket_kernel = esperar_cliente(socket_fs, logger); // quedar a la espera de la conexión por parte del Kernel
    int modulo = recibir_operacion(socket_kernel);

    switch (modulo) {
        case KERNEL:
            log_info(logger, "Kernel Conectado.");
            //enviar_mensaje("HOLA! Generico", socket_kernel);
            recibir_request_kernel(socket_kernel);
            break;
        case -1:
            log_error(logger, "Se desconectó el cliente.");
            close(socket_kernel);
            exit(EXIT_FAILURE);
        default:
            log_error(logger, "Codigo de operacion desconocido.");
            break;
    }
}

void recibir_request_kernel(int socket_kernel) {

	while(true) {
		int resultado;
    	int cod_op = recibir_operacion(socket_kernel); // t_codigo_operacionfs
    	log_debug(logger, "Recibida operación %d", cod_op);
    	char* nombre_archivo = string_new(); //SE RECIBE TAMBIÉN EL NOMBRE DEL ARCHIVO YA QUE ES EL PRIMER PARAMETRO SIEMPRE
    	int tamanio_stream = 0;

    	switch (cod_op) {
			case F_OPEN:
				nombre_archivo = recibir_string(socket_kernel);
				log_debug(logger, "Recibido Archivo %s", nombre_archivo);

				log_debug(logger, "Se recibió un F_OPEN para el archivo %s", nombre_archivo);
				resultado = abrir_archivo(nombre_archivo);
				enviar_entero(socket_kernel, resultado);
				break;
			case F_CREATE:
				nombre_archivo = recibir_string(socket_kernel);
				log_debug(logger, "Recibido Archivo %s", nombre_archivo);

				log_debug(logger, "Se recibió un F_CREATE para el archivo %s", nombre_archivo);
				resultado = crear_archivo(nombre_archivo);
				enviar_entero(socket_kernel, resultado);
				break;
			case F_TRUNCATE:
				nombre_archivo = recibir_string(socket_kernel);
				log_debug(logger, "Recibido Archivo %s", nombre_archivo);

				log_debug(logger, "Se recibió un F_TRUNCATE para el archivo %s", nombre_archivo);
				resultado = truncar_archivo(nombre_archivo);
				enviar_entero(socket_kernel, resultado);
				break;
			case F_READ:
				t_buffer* buffer_read = crear_buffer();
				buffer_read->stream = recibir_buffer(&tamanio_stream, socket_kernel);
				log_debug(logger, "Recibi %d bytes", tamanio_stream);

				nombre_archivo = obtener_string(buffer_read);
				log_debug(logger, "Se recibió un F_READ para el archivo %s", nombre_archivo);

				resultado = leer_archivo(nombre_archivo, buffer_read);
				enviar_entero(socket_kernel, resultado);

				break;
			case F_WRITE:
				t_buffer* buffer = crear_buffer();
				buffer->stream = recibir_buffer(&tamanio_stream, socket_kernel);
				log_debug(logger, "Recibi %d bytes", tamanio_stream);

				nombre_archivo = obtener_string(buffer);
				log_debug(logger, "Se recibió un F_WRITE para el archivo %s", nombre_archivo);

				resultado = escribir_archivo(nombre_archivo, buffer);
				enviar_entero(socket_kernel, resultado);
				break;
			default:
				break;
		}
    }
}

int abrir_archivo(const char* nombreArchivo) {

	log_info(logger, "Abrir Archivo: <%s>", nombreArchivo);

	if(existe_archivo(nombreArchivo)){
		log_debug(logger, "El Archivo <%s> existe.", nombreArchivo);
		return F_OPEN_OK;
	}else{
		log_debug(logger, "El Archivo <%s> no existe.", nombreArchivo);
		return F_NOT_EXISTS;
	}

}

int existe_archivo(char* nombre){
	for(int i=0; i<list_size(lista_fcb);i++){
		t_fcb* fcb= list_get(lista_fcb, i);
		if(string_equals_ignore_case(fcb->NOMBRE_ARCHIVO, nombre)){
			return 1;
		}
	}
	return 0;
}

int crear_archivo(const char* nombreArchivo) {
    log_info(logger, "Crear Archivo: <%s>", nombreArchivo);
	t_fcb* nuevo_fcb = crear_fcb(nombreArchivo);
	list_add(lista_fcb, nuevo_fcb);
    return F_OP_OK;
}

t_fcb* crear_fcb(char* nombre){

	t_fcb* fcb_nuevo= malloc(sizeof(t_fcb));

	fcb_nuevo->NOMBRE_ARCHIVO = string_new();
	string_append(&fcb_nuevo->NOMBRE_ARCHIVO, nombre);
	fcb_nuevo->TAMANIO_ARCHIVO = 0;
	fcb_nuevo->PUNTERO_DIRECTO = 0;
	fcb_nuevo->PUNTERO_INDIRECTO = 0;

	char* path = string_new();

	string_append(&path, fs_config->PATH_FCB);
	string_append(&path, "/");
	string_append(&path, nombre);
	string_append(&path, ".dat");

	FILE* f = fopen(path, "wr");
	fprintf(f, "NOMBRE_ARCHIVO=%s\nTAMANIO_ARCHIVO=0\nPUNTERO_DIRECTO=\nPUNTERO_INDIRECTO=", nombre);
	fclose(f);

	return fcb_nuevo;
}

int truncar_archivo(const char* nombreArchivo) {

	char* paramTamanio = recibir_string(socket_kernel);
	uint32_t nuevoTamanio = atoi(paramTamanio);

	log_info(logger, "Truncar Archivo: <%s> - Tamaño: <%d>", nombreArchivo, nuevoTamanio);

	t_fcb* fcb = obtener_fcb(nombreArchivo);
	log_debug(logger, "Encontrado fcb de %s con tamaño %d", fcb->NOMBRE_ARCHIVO, fcb->TAMANIO_ARCHIVO);
    if (nuevoTamanio == fcb->TAMANIO_ARCHIVO) {
        log_debug(logger, "El archivo <%s> ya tiene el tamaño especificado.", nombreArchivo);
        return F_TRUNCATE_OK;
    }

    if (nuevoTamanio > fcb->TAMANIO_ARCHIVO) {
    	// Aumentar el tamaño del archivo
		int bloquesNecesarios = ceil((nuevoTamanio - fcb->TAMANIO_ARCHIVO) / superbloque->BLOCK_SIZE);
				//ceil_division((nuevoTamanio - fcb->TAMANIO_ARCHIVO), superbloque->BLOCK_SIZE);
		log_debug(logger, "Bloques necesarios: %d", bloquesNecesarios);
		int bloquesAsignados = aumentar_tamanio_archivo(bloquesNecesarios, fcb);
		log_debug(logger, "Bloques asignados: %d", bloquesAsignados);
		if (bloquesAsignados < bloquesNecesarios) {
            log_error(logger, "No hay suficientes bloques disponibles para aumentar el tamaño del archivo.");
            return F_OP_ERROR;
        }
        log_info(logger, "Se aumentó el tamaño del archivo %s a %d bytes.", nombreArchivo, nuevoTamanio);

     } else {
    	int bloquesALiberar = ceil((fcb->TAMANIO_ARCHIVO - nuevoTamanio) / superbloque->BLOCK_SIZE);
    	//int bloquesALiberar = ceil_division((fcb->TAMANIO_ARCHIVO - nuevoTamanio), superbloque->BLOCK_SIZE);
    	int bloquesAsignados = disminuir_tamanio_archivo(bloquesALiberar, fcb);
    	log_info(logger, "Se disminuyó el tamaño del archivo %s a %d bytes.", nombreArchivo, nuevoTamanio);
    }

   	actualizar_fcb(fcb);
   	//imprimir_bitmap(bitmap);
    return F_TRUNCATE_OK;
}

t_fcb* obtener_fcb(char* archivo){
	for(int i=0;i<list_size(lista_fcb);i++){
		t_fcb* fcb = list_get(lista_fcb, i);
		if(string_equals_ignore_case(fcb->NOMBRE_ARCHIVO, archivo)){
			return fcb;
		}
	}
	return NULL;
}

void actualizar_fcb(t_fcb* fcb) {

	char* path = string_new();
	string_append(&path, fs_config->PATH_FCB);
	string_append(&path, "/");
	string_append(&path, fcb->NOMBRE_ARCHIVO);
	string_append(&path, ".dat");

	FILE* f = fopen(path, "wr");
	fprintf(f, "NOMBRE_ARCHIVO=%s\nTAMANIO_ARCHIVO=%d\nPUNTERO_DIRECTO=%d\n", fcb->NOMBRE_ARCHIVO, fcb->TAMANIO_ARCHIVO, fcb->PUNTERO_DIRECTO);
    if (fcb->PUNTERO_INDIRECTO != NULL) {
        fprintf(f, "PUNTERO_INDIRECTO=%d", fcb->PUNTERO_INDIRECTO);
    } else {
    	fprintf(f, "PUNTERO_INDIRECTO=");
    }
	fclose(f);
	free(path);
}

void escribir_en_bloque(uint32_t numero_bloque, void* contenido) {

    uint32_t bloque_size = superbloque->BLOCK_SIZE;
    uint32_t offset = numero_bloque * bloque_size;
    //int cant_a_escribir = strlen((char*) contenido);
    log_debug(logger, "Se va a escribir %d bytes en archivo ", bloque_size);
    FILE* archivo_bloques = fopen(fs_config->PATH_BLOQUES, "rb+");
    if (archivo_bloques == NULL) {
        log_error(logger, "No se pudo abrir el archivo de bloques");
        return;
    }

    fseek(archivo_bloques, offset, SEEK_SET);
    int bytes_escritos = fwrite(contenido, sizeof(char), bloque_size, archivo_bloques);
    if (bytes_escritos != bloque_size) {
        log_error(logger, "Error al escribir en el bloque");
    } else {
    	log_debug(logger, "Contenido escrito en el bloque %u", numero_bloque);
    }
    log_debug(logger, "Se escribieron %d bytes en archivo", bytes_escritos);
    fclose(archivo_bloques);
    sleep(fs_config->RETARDO_ACCESO_BLOQUE/1000);
}

uint32_t obtener_bloque_libre(void) {

	for(int i = 0; i < superbloque->BLOCK_COUNT; i++){
	    if(!bitarray_test_bit(bitmap, i)){ // Bloque disponible
			log_info(logger, "Acceso a Bitmap - Bloque: <%d> - Estado: <LIBRE>", i);
			return i;
		} else {
			log_info(logger, "Acceso a Bitmap - Bloque: <%d> - Estado: <OCUPADO>", i);
		}
	}
	return -1;
}

void reservar_bloque(int index) {
	bitarray_set_bit(bitmap, index);
	log_debug(logger, "Bloque %d reservado", index);
	//imprimir_bitmap(bitmap);
}

void* leer_en_bloques(int posicion, t_fcb* fcb, int index_bloque_archivo) {
	int index_bloque_fs = (posicion / superbloque->BLOCK_SIZE);
	void* datos = malloc(superbloque->BLOCK_SIZE);
	FILE* archivo_bloques = fopen(fs_config->PATH_BLOQUES, "r+b");
	fseek(archivo_bloques, posicion, SEEK_SET);
	size_t bytes_leidos = fread(datos, sizeof(char), superbloque->BLOCK_SIZE, archivo_bloques);
	int cod = feof(archivo_bloques);
	log_debug(logger, "Cod error fread: % d", cod);
	fclose(archivo_bloques);
	// Acceso a Bloque: “Acceso Bloque - Archivo: <NOMBRE_ARCHIVO> - Bloque Archivo: <NUMERO BLOQUE ARCHIVO> - Bloque File System <NUMERO BLOQUE FS>”

	log_info(logger, "Acceso Bloque - Archivo: <%s> - Bloque Archivo: <%d> - Bloque File System <%d>",
			fcb->NOMBRE_ARCHIVO,
			index_bloque_archivo,
			index_bloque_fs);
	sleep(fs_config->RETARDO_ACCESO_BLOQUE / 1000);
	return datos;
}

void liberar_bloque(int index) {
	bitarray_clean_bit(bitmap, index);
	log_debug(logger, "Bloque %d liberado", index);
	//imprimir_bitmap(bitmap);
}

t_bloque* crear_bloque(int bloque_index) {

	t_bloque* bloque = malloc(sizeof(t_bloque));

	int posicion_archivo = bloque_index * superbloque->BLOCK_SIZE;
	bloque->datos = malloc(superbloque->BLOCK_SIZE);
	bloque->inicio = posicion_archivo;
	bloque->fin = posicion_archivo + superbloque->BLOCK_SIZE - 1;
	log_info(logger, "Creado bloque [%d - %d]", bloque->inicio, bloque->fin);
	memset(bloque->datos, 0, superbloque->BLOCK_SIZE);
	reservar_bloque(bloque_index);
	return bloque;
}

t_bloque_indirecto* leer_bloque_indirecto(t_fcb* fcb) {
	log_debug(logger, "Leyendo Bloque Indirecto [Index %d]", fcb->PUNTERO_INDIRECTO);
	t_bloque_indirecto* bloque = malloc(sizeof(t_bloque_indirecto));
	bloque->bloque_propio = malloc(sizeof(t_bloque));
	//bloque->bloque_propio->datos = malloc(superbloque->BLOCK_SIZE);
	bloque->bloque_propio->inicio = fcb->PUNTERO_INDIRECTO * superbloque->BLOCK_SIZE;
	bloque->bloque_propio->fin = bloque->bloque_propio->inicio  + superbloque->BLOCK_SIZE;
	log_debug(logger,"Inicio bloque %d" ,bloque->bloque_propio->inicio);
	log_debug(logger,"Fin bloque %d", bloque->bloque_propio->fin);
	bloque->bloque_propio->datos = (char*)leer_en_bloques(fcb->PUNTERO_INDIRECTO * superbloque->BLOCK_SIZE, fcb, 1);
	bloque->punteros = leer_punteros_bloque_indirecto(bloque->bloque_propio, fcb->TAMANIO_ARCHIVO);
	// completar la lista de punteros;
	sleep(fs_config->RETARDO_ACCESO_BLOQUE/1000);
	return bloque;
}

t_list* leer_punteros_bloque_indirecto(t_bloque* bloque_indirecto, int tamanio_archivo) {
	int punteros_a_leer = ((tamanio_archivo - superbloque->BLOCK_SIZE) / superbloque->BLOCK_SIZE);
	log_debug(logger, "Se leerán %d punteros", punteros_a_leer);
	t_list* lista_punteros = list_create();
	int offset = 0;
	uint32_t puntero = 0;
	//leer_en_bloques(bloque_indirecto->datos, bloque_indirecto->inicio, superbloque->BLOCK_SIZE);
	for (int i = 0; i < punteros_a_leer; i++) {
		memcpy(&puntero, bloque_indirecto->datos + offset, sizeof(uint32_t));
		log_debug(logger, "Leído puntero -> %d", puntero);
		list_add(lista_punteros, puntero);
		offset += sizeof(uint32_t);
	}
	log_debug(logger, "Se leyeron %d punteros", list_size(lista_punteros));
	return lista_punteros;
}

t_bloque_indirecto* crear_bloque_indirecto(int bloque_index) {
	log_debug(logger, "Creando Bloque Indirecto...");
	t_bloque_indirecto* bloque = malloc(sizeof(t_bloque_indirecto));
	bloque->punteros = list_create();
	bloque->bloque_propio = crear_bloque(bloque_index);

	return bloque;
}

void finalizar_fs(int socket_servidor, t_log* logger, t_config* config) {
	log_debug(logger, "Finalizando File System...");
    free(fs_config);
    liberar_conexion(socket_fs);
    log_destroy(logger);
    config_destroy(config);
}

char* leer_datos_archivo(t_fcb* fcb, int puntero, int tamanio_bytes) {

	char* contenido = malloc(tamanio_bytes + 1);
	void* bloques;
	if (fcb->PUNTERO_INDIRECTO != NULL) {
		fcb->bloque_indirecto = leer_bloque_indirecto(fcb);
		bloques = obtener_all_bloques(fcb);
	} else {
		bloques = leer_en_bloques(fcb->PUNTERO_DIRECTO * superbloque->BLOCK_SIZE, fcb, 0);
	}

    memcpy(contenido, bloques + puntero, tamanio_bytes);
    contenido[tamanio_bytes] = '\0';

	free(bloques);
	return contenido;
}

int leer_archivo(char* nombre_archivo, t_buffer* parametros) {

	int direccion_fisica = extraer_int(parametros);
	int tamanio_bytes = extraer_int(parametros);
	int pid = extraer_int(parametros);
	int puntero = extraer_int(parametros);

	log_info(logger, "Leer Archivo: <%s> - Puntero: <%d> - Memoria: <%d> - Tamaño: <%d>", nombre_archivo, puntero,  direccion_fisica, tamanio_bytes);
    t_fcb* fcb = obtener_fcb(nombre_archivo);

    // LEE EL CHOCLO DE BYTES A ENVIAR
    char* a_enviar = leer_datos_archivo(fcb, puntero, tamanio_bytes);
    log_debug(logger, "a enviar : %s", a_enviar);
    // SEND
 	escribir_memoria(pid, direccion_fisica, a_enviar, tamanio_bytes);
 	// RECV
 	char* contenido = recibir_string(socket_memoria);
 	log_debug(logger, "Respuesta memoria: %s", contenido);

 	free(a_enviar);
 	free(contenido);
    //sleep(fs_config->RETARDO_ACCESO_BLOQUE/1000);
	return F_READ_OK;
}

int escribir_archivo(char* nombre_archivo, t_buffer* parametros) {

    int direccion_fisica = extraer_int(parametros);
    int tamanio_bytes = extraer_int(parametros);
    int cantidad_bloques = ceil_division(tamanio_bytes, superbloque->BLOCK_SIZE);
    int pid = extraer_int(parametros);
    int puntero = extraer_int(parametros);
    char* contenido;

    //TODO: PUNTERO
    log_info(logger, "Escribir Archivo: <%s> - Puntero: <%d> - Memoria: <%d> - Tamaño: <%d>",
    		nombre_archivo,
			puntero,
			direccion_fisica,
			tamanio_bytes);

    void* bloques;
    // Obtener el FCB correspondiente al archivo
    t_fcb* fcb = obtener_fcb(nombre_archivo);
    if (fcb->TAMANIO_ARCHIVO > superbloque->BLOCK_SIZE) {
		fcb->bloque_indirecto = leer_bloque_indirecto(fcb);
		bloques = obtener_all_bloques(fcb);
    } else {
    	bloques = leer_en_bloques(fcb->PUNTERO_DIRECTO * superbloque->BLOCK_SIZE, fcb, 0);
    	log_debug(logger, "Leyendo solo bloque directo");
    }
    t_list* punteros = obtener_n_punteros(cantidad_bloques, fcb);

    //Contenido lo tengo que recibir de los registros de memoria
	contenido = leer_memoria(pid, direccion_fisica, tamanio_bytes);

	log_info(logger, "Escribiendo: %s", string_substring_until(contenido, tamanio_bytes));

	memcpy(bloques + puntero, contenido, tamanio_bytes);
	void* bloque64 = malloc(superbloque->BLOCK_SIZE);
	int offset = 0;
	int cantidad_total = list_size(punteros);
	log_debug(logger, "Resincronizando %d bloques, de un total de %d bloques de datos", cantidad_bloques, punteros);
	for(int i= 0; i < cantidad_total; i++) {

		memcpy(bloque64, bloques + offset, superbloque->BLOCK_SIZE);
		offset = offset + superbloque->BLOCK_SIZE;
		uint32_t puntero_i = list_get(punteros, i);
		int index_log = i < 1 ? i : i + 1;
	    log_info(logger, "Acceso Bloque - Archivo: <%s> - Bloque Archivo: <%d> - Bloque File System <%u>",
	    			fcb->NOMBRE_ARCHIVO,
					index_log,
					puntero_i);
		escribir_en_bloque(puntero_i, bloque64);

	}
	free(bloque64);
	//free(bloques);
    return F_WRITE_OK;
}

int aumentar_tamanio_archivo(int bloquesNecesarios, t_fcb *fcb) {
	bool syncronizar_indirecto = false;
	// Buscar bloques disponibles en el bitmap y asignarlos al archivo
	int bloques_asignados = fcb->TAMANIO_ARCHIVO / superbloque->BLOCK_SIZE;
	log_debug(logger, "El fcb tiene %d bloques preexistentes", bloques_asignados); // 0
	log_debug(logger, "El fcb necesita %d bloques más", bloquesNecesarios); // 4
	if (fcb->PUNTERO_INDIRECTO != NULL) {
		log_debug(logger, "Tiene puntero indirecto, sincronizando.");
		fcb->bloque_indirecto = leer_bloque_indirecto(fcb);
	}
	// 3 + 2
	int bloques_procesados = bloques_asignados + bloquesNecesarios;
	while (bloques_asignados < bloques_procesados) {
		switch (bloques_asignados) {
			case 0:
				// Si no tiene asignado un puntero directo
				log_info(logger, "Creando Bloque Directo...");
				fcb->PUNTERO_DIRECTO = obtener_bloque_libre();
				t_bloque *bloque = crear_bloque(fcb->PUNTERO_DIRECTO);
				escribir_en_bloque(fcb->PUNTERO_DIRECTO, bloque->datos);
				fcb->TAMANIO_ARCHIVO = fcb->TAMANIO_ARCHIVO + superbloque->BLOCK_SIZE;
				bloques_asignados++;
				break;
			default:
				// A partir del bloque 2, se revisa si existe el puntero indirecto. Si no, se crea.
				if(fcb->PUNTERO_INDIRECTO == NULL) {
					log_info(logger, "Creando Bloque Indirecto...");
					fcb->PUNTERO_INDIRECTO = obtener_bloque_libre();
					fcb->bloque_indirecto = crear_bloque_indirecto(fcb->PUNTERO_INDIRECTO);
					escribir_en_bloque(fcb->PUNTERO_INDIRECTO, fcb->bloque_indirecto->bloque_propio->datos);
					//fcb->TAMANIO_ARCHIVO = fcb->TAMANIO_ARCHIVO	+ superbloque->BLOCK_SIZE;
				}
				log_info(logger, "Creado Bloque de datos (IS)...");
				uint32_t indice_bloque = obtener_bloque_libre();
				t_bloque* bloque_adicional = crear_bloque(indice_bloque);
				escribir_en_bloque(indice_bloque, bloque_adicional->datos);
				list_add(fcb->bloque_indirecto->punteros, indice_bloque);
				fcb->TAMANIO_ARCHIVO = fcb->TAMANIO_ARCHIVO	+ superbloque->BLOCK_SIZE;
				bloques_asignados++;
				syncronizar_indirecto = true;
				break;
		}
	}
	if (syncronizar_indirecto){
		sincronizar_punteros_bloque_indirecto(fcb);
	}
	return bloques_asignados;
}

void sincronizar_punteros_bloque_indirecto(t_fcb* fcb) {
	log_debug(logger, "Sincronizando punteros a bloque %d[Dir: %d]", (fcb->bloque_indirecto->bloque_propio->inicio/ superbloque->BLOCK_SIZE), fcb->bloque_indirecto->bloque_propio->inicio);
    void* datos_actualizados = obtener_datos_bloque_indirecto(fcb->bloque_indirecto);
    log_debug(logger, "Escribiendo en bloque indirecto con datos de tamaño %zu...", sizeof(datos_actualizados));
    //log_info(logger, "Datos: %s", (char*)datos_actualizados);
    escribir_en_bloque(fcb->PUNTERO_INDIRECTO, datos_actualizados);
    log_info(logger, "Acceso Bloque - Archivo: <%s> - Bloque Archivo: <%d> - Bloque File System <%d>",
    			fcb->NOMBRE_ARCHIVO,
    			1,
				fcb->PUNTERO_INDIRECTO);
    free(datos_actualizados);
}

void* obtener_datos_bloque_indirecto(t_bloque_indirecto* bloque_indirecto) {
	int cantidad_punteros = list_size(bloque_indirecto->punteros);
	int tamano_datos = cantidad_punteros * sizeof(uint32_t);

	void* datos = malloc(superbloque->BLOCK_SIZE);
	log_debug(logger, "Se van a guardar %d punteros [%d bytes]", cantidad_punteros, tamano_datos);
	int offset = 0;
	for (int i = 0; i < cantidad_punteros; i++) {
		uint32_t puntero = (uint32_t)list_get(bloque_indirecto->punteros, i);
		log_debug(logger, "Guardando puntero %d...", puntero);
		memcpy(datos + offset, &puntero, sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}
	int bytes_restantes = superbloque->BLOCK_SIZE - tamano_datos;
	log_debug(logger, "Bytes restantes a completar: %d", bytes_restantes);
	if (bytes_restantes > 0) {
		memset(datos + offset, 9, bytes_restantes);
	}
	return datos;
}

int disminuir_tamanio_archivo(int bloquesALiberar, t_fcb *fcb) {

	int bloquesAsignados = fcb->TAMANIO_ARCHIVO / superbloque->BLOCK_SIZE;
	log_debug(logger, "Bloques Asignados: %d", bloquesAsignados);
	log_debug(logger, "Bloques A Liberar: %d", bloquesALiberar);
	t_bloque_indirecto* bloque_indirecto;
	int cant_punteros;
	if (bloquesAsignados > 1) {
		bloque_indirecto = leer_bloque_indirecto(fcb);
		cant_punteros = list_size(bloque_indirecto->punteros);
	}
	for (int i = 1; i <= bloquesALiberar; i++) {
		// 3
		log_debug(logger, "Loop %d", i);
		if (bloquesAsignados == 1) {
			liberar_bloque(fcb->PUNTERO_DIRECTO);
		} else { //TODO:REVISAR
			log_debug(logger, "List_get(..., %d)", cant_punteros - i);
			log_debug(logger, "Cantidad de bloques IS: %d", cant_punteros);
			int bloque_index = list_get(bloque_indirecto->punteros, cant_punteros - i);
			log_debug(logger, "Eliminando bloque %d", bloque_index);
			liberar_bloque(bloque_index);
			list_remove(bloque_indirecto->punteros, cant_punteros - i);

			if (bloquesAsignados == 2) {
				liberar_bloque(fcb->PUNTERO_INDIRECTO);
				fcb->PUNTERO_INDIRECTO = NULL;
				list_destroy(bloque_indirecto->punteros); // se elimina lista de bloques indirectos interna del fcb
				free(bloque_indirecto); // todo: hacer un destroy
			}
		}
		fcb->TAMANIO_ARCHIVO = fcb->TAMANIO_ARCHIVO - superbloque->BLOCK_SIZE;
		bloquesAsignados -= 1;
	}
	return bloquesAsignados;
}

void* obtener_all_bloques(t_fcb* fcb) {

	int offset = 0;
	int punteros_size = (list_size(fcb->bloque_indirecto->punteros) > 0) ? list_size(fcb->bloque_indirecto->punteros) : 1;
	log_debug(logger, "Cantidad de bloques indirectos %d", punteros_size);
	// la cantidad de bloques indirectos + bloque directo
	void* bloques = malloc(superbloque->BLOCK_SIZE * punteros_size + superbloque->BLOCK_SIZE);
	log_debug(logger, "Size malloc: %d", superbloque->BLOCK_SIZE * punteros_size + superbloque->BLOCK_SIZE);

	memcpy(bloques, leer_en_bloques(fcb->PUNTERO_DIRECTO * superbloque->BLOCK_SIZE, fcb, 0), superbloque->BLOCK_SIZE);
	offset = offset + superbloque->BLOCK_SIZE;

	if(punteros_size > 1) {
		//printf("Necesita mas de un bloque \n");
		for(int i = 0; i < punteros_size; i++) {
            int puntero = list_get(fcb->bloque_indirecto->punteros, i);
            log_debug(logger, "leyendo puntero %d", puntero);
            void* bloque = leer_en_bloques(puntero * superbloque->BLOCK_SIZE, fcb, i + 2); // i + 2 para loggear desde el primer bloque de datos IS
            memcpy(bloques + offset, bloque, superbloque->BLOCK_SIZE);
            offset += superbloque->BLOCK_SIZE;
		}
	}

	return bloques;
}

t_list* obtener_n_punteros(int cantidad_bloques, t_fcb* fcb){
	t_list* punteros = list_create();
	log_debug(logger, "Enlistando %d punteros + el puntero directo", cantidad_bloques);
	list_add(punteros, fcb->PUNTERO_DIRECTO);

	if(cantidad_bloques > 1){
		for(int i =0; i < list_size(fcb->bloque_indirecto->punteros); i++) {
			list_add(punteros, list_get(fcb->bloque_indirecto->punteros, i));
		}
	}
	return punteros;
}

int ceil_division(int param1, int param2) {
    int valor = param1 / param2;
    if (param1 % param2 > 0) {
        valor++;
    }
    return valor;
}

char* leer_memoria(int pid, int direccion_fisica, int cant_de_bytes) {
	t_paquete* paquete = crear_paquete(MEMORY_READ_ADRESS);
	paquete->buffer = crear_buffer();
	agregar_int_a_paquete(paquete, pid);
	agregar_int_a_paquete(paquete, direccion_fisica);
	agregar_int_a_paquete(paquete, cant_de_bytes);
	enviar_paquete(paquete, socket_memoria);
	eliminar_paquete(paquete);

	return recibir_string(socket_memoria);
}

void escribir_memoria(int pid, int direccion_fisica, char* valor_a_escribir, int tamanio) {
	t_paquete* paquete = crear_paquete(MEMORY_WRITE_ADRESS);
	paquete->buffer = crear_buffer();
	agregar_int_a_paquete(paquete, pid);
	agregar_int_a_paquete(paquete, direccion_fisica);
	agregar_int_a_paquete(paquete, tamanio);
	agregar_a_paquete(paquete, valor_a_escribir, tamanio);
	enviar_paquete(paquete, socket_memoria);
	eliminar_paquete(paquete);
}

