#ifndef FILE_PARSER_H_
#define FILE_PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <shared.h>

#define SIZE_PROGRAM 216;

t_programa* parsear_programa(char *, t_log*);
bool parsear(t_programa*, FILE*, t_log*);
int parsear_instrucciones(char*, t_list*, t_log*);
void liberar_memoria_parseo(char **, char *);
#endif /* FILE_PARSER_H_ */
