#include <errno.h>
#include "shared.h"
#include "estructuras.h"

t_instruccion* crear_instruccion(t_codigo_instruccion codigo, bool empty) {
	t_instruccion* instruccion = malloc(sizeof(t_instruccion));
	instruccion->codigo = codigo;

	if (empty)
		instruccion->parametros = NULL;
	else
		instruccion->parametros = list_create();

	return instruccion;
}

void buffer_destroy(t_buffer* buffer) {
	free(buffer->stream);
	free(buffer);
}

t_buffer* serializar_instrucciones(t_list* instrucciones, t_log* logger) {

	t_buffer* buffer; //buffer a retornar
	int size_buffer = 0; // tamaño total del buffer a retornar
	int cant_instrucciones = 0; // cantidad de instrucciones
	t_list_iterator* iterador_instrucciones; // puntero para recorrer las instrucciones del programa
	t_list_iterator* iterador_parametros; // puntero para recorrer los parametros de una instruccion
	int cant_parametros = 0; // Cantidad de parámetros para validar existencia
	t_instruccion* instruccion; // La instrucción actual que se va a recorrer
	int antes = 0;

	iterador_instrucciones = list_iterator_create(instrucciones);
	// Mientras exista una instrucción más
	while (list_iterator_has_next(iterador_instrucciones)) {

		instruccion = (t_instruccion*)list_iterator_next(iterador_instrucciones);
		//log_info(logger, "El código OP a alocar es: %d", instruccion->codigo);

		size_buffer += sizeof(int) + sizeof(int); // codigo de instruccion + cantidad de parametros
		cant_parametros  = list_size(instruccion->parametros);

		// Si tiene parámetros
		if (cant_parametros > 0) {
			iterador_parametros = list_iterator_create(instruccion->parametros);
			// Mientras exista otro parámetro más
			while(list_iterator_has_next(iterador_parametros)) {
				// Tamaño del parámetro + Tamaño del String + 1 por el endline
				antes = size_buffer;
				size_buffer += sizeof(int) + strlen((char*)list_iterator_next(iterador_parametros)) + 1;
//				log_info(logger, "Este parámetro de instruccion pesa %d", size_buffer - antes - 5);
			}
			// Liberamos el iterador de parámetros
			list_iterator_destroy(iterador_parametros);
		}
		// Contamos una instrucción más.
		cant_instrucciones++;
	}
	// Liberamos el iterador de instrucciones
	list_iterator_destroy(iterador_instrucciones);
	//log_info(logger, "Análisis de serializado: %d Instrucciones, con un tamaño de buffer total de: %d", cant_instrucciones, size_buffer);
	/*
	 * SERIALIZACION
	 * */
	//t_list_iterator* iterador_instrucciones; // puntero para recorrer las instrucciones del programa
	//t_list_iterator* iterador_parametros; // puntero para recorrer los parametros de una instruccion
	//t_instruccion* instruccion; // La instruccion a recorrer
	char *parametro; // El parametro a recorrer
	int offset = 0; // desfasaje/corrimimento de bytes en el buffer
	int size_parametro; // Tamaño del parámetro
	//int cant_parametros = 0; // Cantidad de Parámetros

	buffer = crear_buffer();
	// Es el Tamaño calculado + el int con las cantidad de instrucciones a enviar
	buffer->size = size_buffer + sizeof(int);
	buffer->stream = malloc(buffer->size);

	//MEMCPY(DESTINO, FUENTE, TAMAÑO)
	//Cantidad de instrucciones
	memcpy(buffer->stream + offset, &(cant_instrucciones), sizeof(int));
	offset += sizeof(int);
	iterador_instrucciones = list_iterator_create(instrucciones);
	//log_info(logger, "Se encontró una lista de: %d",list_size(instrucciones));


	while (list_iterator_has_next(iterador_instrucciones)) {
		instruccion = (t_instruccion*)list_iterator_next(iterador_instrucciones);
//		log_info(logger, "Entranado al index: %d",iterador_instrucciones->index);
		// Código de instrucción
		//log_info(logger, "El código OP a alocar es: %d", instruccion->codigo);
		memcpy(buffer->stream + offset, &(instruccion->codigo), sizeof(int));
		offset += sizeof(int);
		// Cantidad de parámetros
		cant_parametros = list_size(instruccion->parametros);
//		log_info(logger, "voy a alocar %d parametros", cant_parametros);
		memcpy(buffer->stream + offset, &cant_parametros, sizeof(int));
		offset += sizeof(int);
		// Si tiene parámetros
		if (cant_parametros > 0) {
			iterador_parametros = list_iterator_create(instruccion->parametros);
			// Mientras existra otro parámetro, tal vez queda medio redundante con el if de arriba.
			while (list_iterator_has_next(iterador_parametros)) {
				parametro = (char*) list_iterator_next(iterador_parametros);
//				log_info(logger, "el parámetro %s, mide %d", parametro, (int)(strlen(parametro) + 1));
				size_parametro = strlen(parametro) + 1;
				// Tamaño del parámetro
				memcpy(buffer->stream + offset, &(size_parametro), sizeof(int));
				offset += sizeof(int);
				// Parámetro
				memcpy(buffer->stream + offset, parametro, size_parametro);
				offset += size_parametro;
			}
			list_iterator_destroy(iterador_parametros);
		}
	}
	list_iterator_destroy(iterador_instrucciones);


	if (offset != buffer->size)
		log_error(logger, "El tamaño del buffer[%d] no coincide con el offset[%d]\n", (int)buffer->size, offset);
	else{
//		log_info(logger, "El tamaño del buffer[%d] coincide con el offset[%d]\n", (int)buffer->size, offset);
	}
	//int offset = serializar_buffer_programa(size_buffer, cant_instrucciones, instrucciones);


	return buffer;
}

t_list* deserializar_instrucciones(t_buffer* buffer, t_log* logger) {
	t_list* instrucciones = list_create();
	t_instruccion* instruccion;
	int cant_instrucciones;
	int cant_parametros;
	int size_parametro;
	char* parametro;
	t_codigo_instruccion cod_instruccion;
	int offset = 0;

//	log_info(logger,"Se van a deserializar las instrucciones");
	// cantidad de instrucciones
	memcpy(&cant_instrucciones, buffer->stream + offset, sizeof(int));
	offset += sizeof(int);

	// por cada instrucción que tenga el programa
	for (int i = 0; i < cant_instrucciones; i++) {

		//Código de instrucción
		memcpy(&(cod_instruccion), buffer->stream + offset, sizeof(int));
		offset += sizeof(int);

		instruccion = crear_instruccion(cod_instruccion, false);

		//Cantidad de parámetros
		memcpy(&(cant_parametros), buffer->stream + offset, sizeof(int));
		offset += sizeof(int);

		// Los casos de YIELD, EXIT no tienen parámetros y quedan con instruccion->parametros = NULL;
		if(cant_parametros > 0) {
			// por cada parámetro que tenga la instrucción
			for (int j = 0; j < cant_parametros; j++) {

				// Size del parámetro
				memcpy(&(size_parametro), buffer->stream + offset, sizeof(int));
				offset += sizeof(int);

				// Parámetro
				parametro = malloc(size_parametro);
				memcpy(parametro, buffer->stream + offset, size_parametro);
				offset += size_parametro;

				// Se inserta parámetro a la lista de parámetros
				list_add(instruccion->parametros, parametro);
			}
		}
		// Se inserta instrucción a la lista de instrucciones
		list_add(instrucciones,instruccion);
	}
	return instrucciones;
}

t_buffer* serializar_tabla_segmentos(t_list* tabla_segmentos) {
	t_buffer* buffer = crear_buffer(); //buffer a retornar
	int cant_segmentos = list_size(tabla_segmentos);
	int offset = 0;

	// TAMANIIO SEGMENTO * CANT_SEGMENTO + NUMERO CANTIDAD DE SEGMENTOS
	buffer->size = cant_segmentos * 4 * sizeof(uint32_t) + sizeof(int);
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream, &(cant_segmentos), sizeof(int));
	offset += sizeof(int);

	for(int i=0; i< cant_segmentos; i++){
		t_segmento* segmento = (t_segmento*) list_get(tabla_segmentos, i);

		memcpy(buffer->stream + offset, &(segmento->descriptor_id), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(buffer->stream + offset, &(segmento->segmento_id), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(buffer->stream + offset, &(segmento->inicio), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(buffer->stream + offset, &(segmento->tam_segmento), sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	return buffer;
}

t_list* deserializar_tabla_segmentos(void* stream){
	t_list* segmentos = list_create();
	int cant_segmentos;
	int offset = 0;

	memcpy(&(cant_segmentos), stream + offset, sizeof(int));
	offset += sizeof(int);
	for(int i=0; i< cant_segmentos; i++){
		t_segmento* segmento = malloc(sizeof(t_segmento));
		memcpy(&(segmento->descriptor_id), stream + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(&(segmento->segmento_id), stream + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(&(segmento->inicio), stream + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(&(segmento->tam_segmento), stream + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		list_add(segmentos,segmento);
	}
	free(stream);
	return segmentos;
}

void enviar_contexto(int socket, t_contexto_proceso* contexto, int codigo, t_log* logger){

	t_paquete* paquete = crear_paquete(codigo);
	t_buffer* buffer_instrucciones = serializar_instrucciones(contexto->instrucciones,logger);
	t_buffer* buffer_tabla_segmentos = serializar_tabla_segmentos(contexto->tabla_segmentos);
	t_buffer* buffer_paquete = crear_buffer();
	paquete->codigo_operacion = codigo;

	int offset = 0;
	// PID + PC + Size de Instrucciones + size_segmentos
	int tamanio_paquete = sizeof(int) *4 + size_of_registros(contexto) + buffer_instrucciones->size + buffer_tabla_segmentos->size;
//	log_info(logger, "Tamaño Paquete: %d", tamanio_paquete);
//	log_info(logger, "Tamaño PID + PC + Size Instrucciones: %d", sizeof(int)*3);
//	log_info(logger, "Tamaño Instrucciones: %d", buffer_instrucciones->size);
	paquete->buffer = buffer_paquete;
	paquete->buffer->size = tamanio_paquete;
	paquete->buffer->stream = malloc(tamanio_paquete);

	memcpy(paquete->buffer->stream + offset, &(contexto->pid), sizeof(int));
	offset+=sizeof(int);
	memcpy(paquete->buffer->stream + offset, &(contexto->program_counter), sizeof(int));
	offset+=sizeof(int);
	//size_stream_instrucciones_total
	memcpy(paquete->buffer->stream + offset, &(buffer_instrucciones->size),sizeof(int));
	offset+=sizeof(int);
	//cant_instrucciones.stream_instrucciones
	memcpy(paquete->buffer->stream + offset, buffer_instrucciones->stream, buffer_instrucciones->size);
	offset+= buffer_instrucciones->size;

	memcpy(paquete->buffer->stream + offset, &(buffer_tabla_segmentos->size),sizeof(int));
	offset+=sizeof(int);
	memcpy(paquete->buffer->stream + offset, buffer_tabla_segmentos->stream, buffer_tabla_segmentos->size);
	offset+= buffer_tabla_segmentos->size;

	//serializar_registros(contexto->registros, paquete->buffer->stream, &offset);

    memcpy(paquete->buffer->stream + offset, contexto->registros.AX, sizeof(contexto->registros.AX));
//    log_info(logger, "Contexto AX: %s",contexto->registros.AX);
    offset += sizeof(contexto->registros.AX);

    memcpy(paquete->buffer->stream + offset, contexto->registros.BX, sizeof(contexto->registros.BX));
    offset += sizeof(contexto->registros.BX);

    memcpy(paquete->buffer->stream + offset, contexto->registros.CX, sizeof(contexto->registros.CX));
    offset += sizeof(contexto->registros.CX);
//    log_info(logger, "Contexto CX: %s",contexto->registros.CX);

    memcpy(paquete->buffer->stream + offset, contexto->registros.DX, sizeof(contexto->registros.DX));
    offset += sizeof(contexto->registros.DX);

    memcpy(paquete->buffer->stream + offset, contexto->registros.EAX, sizeof(contexto->registros.EAX));
    offset += sizeof(contexto->registros.EAX);

    memcpy(paquete->buffer->stream + offset, contexto->registros.EBX, sizeof(contexto->registros.EBX));
    offset += sizeof(contexto->registros.EBX);

    memcpy(paquete->buffer->stream + offset, contexto->registros.ECX, sizeof(contexto->registros.ECX));
    offset += sizeof(contexto->registros.ECX);

    memcpy(paquete->buffer->stream + offset, contexto->registros.EDX, sizeof(contexto->registros.EDX));
    offset += sizeof(contexto->registros.EDX);

    memcpy(paquete->buffer->stream + offset, contexto->registros.RAX, sizeof(contexto->registros.RAX));
    offset += sizeof(contexto->registros.RAX);
//    log_info(logger, "Contexto RAX: %s",contexto->registros.RAX);

    memcpy(paquete->buffer->stream + offset, contexto->registros.RBX, sizeof(contexto->registros.RBX));
    offset += sizeof(contexto->registros.RBX);

    memcpy(paquete->buffer->stream + offset, contexto->registros.RCX, sizeof(contexto->registros.RCX));
    offset += sizeof(contexto->registros.RCX);

    memcpy(paquete->buffer->stream + offset, contexto->registros.RDX, sizeof(contexto->registros.RDX));
    offset += sizeof(contexto->registros.RDX);

//	log_info(logger, "Offset = %d", offset);
	enviar_paquete(paquete, socket);

	free(buffer_instrucciones->stream);
	free(buffer_instrucciones);
	free(buffer_tabla_segmentos->stream);
	free(buffer_tabla_segmentos);
	eliminar_paquete(paquete);
}

t_list* recibir_tabla_de_segmentos(int socket){
	int size;
	void* stream;

	recv(socket, &size, sizeof(int), MSG_WAITALL);
	stream = malloc(size);
	recv(socket, stream, size, MSG_WAITALL);

	return deserializar_tabla_segmentos(stream);
}

int size_of_registros(t_contexto_proceso* contexto) {
	int size = 0;
	size += sizeof(contexto->registros.AX);
	size += sizeof(contexto->registros.BX);
	size += sizeof(contexto->registros.CX);
	size += sizeof(contexto->registros.DX);
	size += sizeof(contexto->registros.EAX);
	size += sizeof(contexto->registros.EBX);
	size += sizeof(contexto->registros.ECX);
	size += sizeof(contexto->registros.EDX);
	size += sizeof(contexto->registros.RAX);
	size += sizeof(contexto->registros.RBX);
	size += sizeof(contexto->registros.RCX);
	size += sizeof(contexto->registros.RDX);
	//printf("los registros pesan %d \n", size);
	return size;
}

t_contexto_proceso* recibir_contexto(int socket,t_log* logger){

	int size = 0 ;
	int desplazamiento = 0;
	void * buffer;
	t_contexto_proceso* proceso = malloc(sizeof(t_contexto_proceso));
	t_buffer* buffer_instrucciones = malloc(sizeof(t_buffer));
	int size_stream_segmentos;

	buffer = recibir_buffer(&size,socket);
	//log_info(logger, "Recibí un buffer de: %d", size);
	memcpy(&(proceso->pid),buffer + desplazamiento,sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(&(proceso->program_counter),buffer + desplazamiento,sizeof(int));
	desplazamiento+=sizeof(int);

	memcpy(&(buffer_instrucciones->size),buffer + desplazamiento,sizeof(int));
	desplazamiento+=sizeof(int);
	buffer_instrucciones->stream = malloc(buffer_instrucciones->size);
	memcpy((buffer_instrucciones->stream),buffer + desplazamiento,buffer_instrucciones->size);
	proceso->instrucciones = deserializar_instrucciones(buffer_instrucciones,logger);
	desplazamiento+=buffer_instrucciones->size;

    memcpy(&size_stream_segmentos, buffer + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);
	void* stream_segmentos = malloc(size_stream_segmentos);
	memcpy(stream_segmentos, buffer + desplazamiento, size_stream_segmentos);
	proceso->tabla_segmentos = deserializar_tabla_segmentos(stream_segmentos);
	desplazamiento += size_stream_segmentos;

	//memcpy(&(proceso->registros),buffer + desplazamiento, sizeof(t_registro));
    memcpy(&(proceso->registros.AX), buffer + desplazamiento, sizeof(proceso->registros.AX));
    desplazamiento += sizeof(proceso->registros.AX);

    memcpy(&(proceso->registros.BX), buffer + desplazamiento, sizeof(proceso->registros.BX));
    desplazamiento += sizeof(proceso->registros.BX);

    memcpy(&(proceso->registros.CX), buffer + desplazamiento, sizeof(proceso->registros.CX));
    desplazamiento += sizeof(proceso->registros.CX);

    memcpy(&(proceso->registros.DX), buffer + desplazamiento, sizeof(proceso->registros.DX));
    desplazamiento += sizeof(proceso->registros.DX);

    memcpy(&(proceso->registros.EAX), buffer + desplazamiento, sizeof(proceso->registros.EAX));
    desplazamiento += sizeof(proceso->registros.EAX);

    memcpy(&(proceso->registros.EBX), buffer + desplazamiento, sizeof(proceso->registros.EBX));
    desplazamiento += sizeof(proceso->registros.EBX);

    memcpy(&(proceso->registros.ECX), buffer + desplazamiento, sizeof(proceso->registros.ECX));
    desplazamiento += sizeof(proceso->registros.ECX);

    memcpy(&(proceso->registros.EDX), buffer + desplazamiento, sizeof(proceso->registros.EDX));
    desplazamiento += sizeof(proceso->registros.EDX);

    memcpy(&(proceso->registros.RAX), buffer + desplazamiento, sizeof(proceso->registros.RAX));
    desplazamiento += sizeof(proceso->registros.RAX);

    memcpy(&(proceso->registros.RBX), buffer + desplazamiento, sizeof(proceso->registros.RBX));
    desplazamiento += sizeof(proceso->registros.RBX);

    memcpy(&(proceso->registros.RCX), buffer + desplazamiento, sizeof(proceso->registros.RCX));
    desplazamiento += sizeof(proceso->registros.RCX);

    memcpy(&(proceso->registros.RDX), buffer + desplazamiento, sizeof(proceso->registros.RDX));
    desplazamiento += sizeof(proceso->registros.RDX);

	free(buffer_instrucciones->stream);
	free(buffer_instrucciones);
	free(buffer);

	//log_info(logger, "Se recibio un proceso con PID: %d",proceso->pid);
	return proceso;
}

void loggear_segmentos(t_list* lista_segmentos, t_log* logger){
	log_debug(logger,"----------SEGMENTOS--------");
	log_debug(logger,"D_ID	ID	INICIO	FIN	TAMANIO");
	void _log(void* elem){
		t_segmento* seg  = (t_segmento*) elem;

		log_debug(logger,"%d	%d	%d	%d	%d",seg->descriptor_id,seg->segmento_id,seg->inicio,seg->inicio + seg->tam_segmento - 1,seg->tam_segmento);
	}

	list_iterate(lista_segmentos,&_log);
}

