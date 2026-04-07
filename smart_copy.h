#ifndef SMART_COPY_H
#define SMART_COPY_H

#include <time.h>

#define BUFFER_SIZE 4096

typedef struct {
    int buffLeidos;
    int buffEscritos;
    long bytesTotal;
    int errores;
}CargadoCopia;

typedef struct{
    double tSegudosCopia;
    double velocidadCopia; 
    CargadoCopia reg;
}ResultadoCopia;

ResultadoCopia copia_buffer(const char *origen, const char *destino);
ResultadoCopia copia_libreria(const char *origen, const char *destino);

#endif 