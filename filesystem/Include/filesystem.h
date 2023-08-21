#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <shared.h>
#include <pthread.h>
#include <fcntl.h>
#include <dirent.h>
#include <commons/config.h>
#include <commons/txt.h>
#include "estructuras.h"
#include <math.h>

/* ESTRUCTURAS */
typedef struct {
    char* IP_MEMORIA;
    char* PUERTO_MEMORIA;
    char* PUERTO_ESCUCHA;
    char* PATH_SUPERBLOQUE;
    char* PATH_BITMAP;
    char* PATH_BLOQUES;
    char* PATH_FCB;
    int RETARDO_ACCESO_BLOQUE;
} t_fs_config;

typedef struct {
    int BLOCK_COUNT;
    int BLOCK_SIZE;
} Superbloque;

typedef struct {
	char* datos;
	int inicio;
	int fin;
} t_bloque;

typedef struct {
    t_list* punteros; // uint32_t
    t_bloque* bloque_propio;
} t_bloque_indirecto;

typedef struct {
    char* NOMBRE_ARCHIVO;
    int TAMANIO_ARCHIVO;
    uint32_t PUNTERO_DIRECTO;
    uint32_t PUNTERO_INDIRECTO;
    t_bloque_indirecto* bloque_indirecto;
} t_fcb;


/* CONSTANTES */
#define MAX_ARCHIVOS 50


/* VARIABLES */
t_config* config;
t_config* ip_config;
int socket_kernel;
int socket_memoria;
int socket_fs;
char* mapBitmap;
t_bitarray* bitmap;
t_fs_config* fs_config;
Superbloque* superbloque;
t_fcb* fcb;
void* bloques;
t_bitarray* bloques_bitarray;
t_fcb lista_archivos[100];
int arch_en_mem;

t_list* lista_fcb;

/* PROCEDIMIENTOS */
void cargar_config_fs(t_config* config);
void conectar_con_memoria();
void iniciar_fs();
void crear_directorio(char* path);
void iniciar_superbloque();
void iniciar_bitmap();
void iniciar_bloques();
void correr_servidor();
void recibir_request_kernel(int socket_kernel);
int leer_archivo(char* nombre_archivo, t_buffer* parametros);
int escribir_archivo(char* nombre_archivo, t_buffer* parametros);
void levantar_fcb(const char* nombre_archivo);
void cargar_config_fcb(t_config* config_file);
void actualizar_fcb(t_fcb* fcb);
void persistir_fcb(t_fcb* archivo);
void accederBitmap(uint32_t numeroBloque, int estado);
void accederBloque(const char* nombreArchivo, uint32_t numeroBloqueArchivo, uint32_t numeroBloqueFS);
void finalizar_fs(int conexion, t_log* logger, t_config* config);
void liberar_bloque(int index);
void reservar_bloque(int index);
void iniciar_FCBs();
void escribir_en_bloque(uint32_t numero_bloque, void* contenido);
/* FUNCIONES */
int existe_fs();
int abrir_archivo(const char* nombreArchivo);
int crear_archivo(const char* nombreArchivo);
int truncar_archivo(const char* nombreArchivo);
int aumentar_tamanio_archivo(int bloquesNecesarios, t_fcb *fcb);
int disminuir_tamanio_archivo(int bloquesALiberar, t_fcb *fcb);
uint32_t obtener_bloque_libre(void);
t_bloque* crear_bloque(int bloque_index);
t_bloque_indirecto* crear_bloque_indirecto(int bloque_index);
int cargar_archivos(t_fcb* lis_archivos);
t_bloque* obtener_bloque(int bloque_index);
t_fcb* leer_fcb(char* nombre, uint32_t tamanio, uint32_t puntero_directo, uint32_t puntero_indirecto);
t_fcb* crear_fcb(char* nombre);
void imprimir_bitmap(t_bitarray* bitmap);
t_fcb* obtener_fcb(char* archivo);
int existe_archivo(char* nombre);
void* leer_en_bloques(int posicion, t_fcb* fcb, int index_bloque_archivo);
t_list* obtener_n_punteros(int cantidad_bloques, t_fcb* fcb);
void* obtener_all_bloques(t_fcb* fcb);
void sincronizar_punteros_bloque_indirecto(t_fcb* fcb);
void* obtener_datos_bloque_indirecto(t_bloque_indirecto* bloque_indirecto);
t_bloque_indirecto* leer_bloque_indirecto(t_fcb* fcb);
t_list* leer_punteros_bloque_indirecto(t_bloque* bloque_indirecto, int tamanio_archivo);
char* leer_datos_archivo(t_fcb* fcb, int puntero, int tamanio_bytes);
int ceil_division(int param1, int param2);

char* leer_memoria(int pid, int direccion_fisica, int cant_de_bytes);
void escribir_memoria(int pid, int direccion_fisica, char* valor_a_escribir, int tamanio);
#endif /* FILESYSTEM_H_ */
