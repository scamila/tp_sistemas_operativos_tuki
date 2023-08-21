#include "../Include/consola.h"

int main(int argc, char** argv) {

	if (argc < 3) {
		return EXIT_FAILURE;
	}

	char* archivo_config = argv[1];
	char* archivo_programa = argv[2];

	correr_consola(archivo_config, archivo_programa);

	return EXIT_SUCCESS;
}
