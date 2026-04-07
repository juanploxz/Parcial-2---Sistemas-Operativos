#include "smart_copy.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#define MAX_INTENTOS 3 // Veces que se puede reintentar la copia antes de darla por fallida

// Manejo de errores
static void manejar_errno(const char *contexto) {
    switch (errno) {
        case ENOENT:
            fprintf(stderr, "[ERROR - %s] Ruta de archivo invalida (errno %d: %s)\n",
                    contexto, errno, strerror(errno));
            break;
        case EACCES:
            fprintf(stderr, "[ERROR - %s] Verifique permisos del archivo. (errno %d: %s)\n",
                    contexto, errno, strerror(errno));
            break;
        case ENOSPC:
            fprintf(stderr, "[ERROR - %s] Disco lleno, liberar espacio. (errno %d: %s)\n",
                    contexto, errno, strerror(errno));
            break;
        case EISDIR:
            fprintf(stderr, "[ERROR - %s] La direccion escrita es un directorio. (errno %d: %s)\n",
                    contexto, errno, strerror(errno));
            break;
        case EROFS:
            fprintf(stderr, "[ERROR - %s] Sistema de archivos read only. (errno %d: %s)\n",
                    contexto, errno, strerror(errno));
            break;
        case EMFILE:
            fprintf(stderr, "[ERROR - %s] Demasiados archivos abiertos por el proceso. (errno %d: %s)\n",
                    contexto, errno, strerror(errno));
            break;
        case ENFILE:
            fprintf(stderr, "[ERROR - %s] Demasiados archivos abiertos en el sistema. (errno %d: %s)\n",
                    contexto, errno, strerror(errno));
            break;
        case EIO:
            fprintf(stderr, "[ERROR - %s] Error de entrada/salida en el dispositivo. (errno %d: %s)\n",
                    contexto, errno, strerror(errno));
            break;
        default:
            fprintf(stderr, "[ERROR - %s] Error inesperado. (errno %d: %s)\n",
                    contexto, errno, strerror(errno));
            break;
    }
}

//Copia mediante buffer de 4096 bytes.
ResultadoCopia copia_buffer(const char *origen, const char *destino) {
    ResultadoCopia res = {0};
    int intentos = 0;
    int copiaExitosa = 0;

    // Abrir direccion de origen 
    int fd_origen = open(origen, O_RDONLY);
    if (fd_origen < 0) {
        manejar_errno("open origen [syscall]");
        res.reg.errores++;
        return res;
    }

    long Torigen = lseek(fd_origen, 0, SEEK_END);
    lseek(fd_origen, 0, SEEK_SET);

    // Ciclo para repetir hasta cumplir con maximo de intentos o lograr una copia exitosa
    while (intentos < MAX_INTENTOS && !copiaExitosa) {
        intentos++;
        res.reg.buffLeidos  = 0;
        res.reg.buffEscritos = 0;
        res.reg.bytesTotal  = 0;
        lseek(fd_origen, 0, SEEK_SET);

        // Abrir destino
        int fd_destino = open(destino, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_destino < 0) {
            manejar_errno("open destino [syscall]");
            res.reg.errores++;
            continue;
        }

        clock_t inicio = clock();//inicializacion de conteo de tiempo
        char buffer[BUFFER_SIZE];
        ssize_t bytes_leidos;
        int error_int = 0;

        // Bucle de lectura/escritura
        while ((bytes_leidos = read(fd_origen, buffer, BUFFER_SIZE)) > 0) {
            res.reg.buffLeidos++;

            ssize_t bytes_escritos = write(fd_destino, buffer, bytes_leidos);
            if (bytes_escritos < 0) {
                manejar_errno("write [syscall]");
                res.reg.errores++;
                error_int = 1;
                break;
            }
            // Escritura parcial (ej: disco casi lleno)
            if (bytes_escritos < bytes_leidos) {
                errno = ENOSPC;
                manejar_errno("No se pudo completar la escritura [syscall]");
                res.reg.errores++;
                error_int = 1;
                break;
            }

            res.reg.buffEscritos++;
            res.reg.bytesTotal += bytes_escritos;
        }

        // Error en la lectura
        if (bytes_leidos < 0) {
            manejar_errno("read [syscall]");
            res.reg.errores++;
            error_int = 1;
        }

        close(fd_destino);

        clock_t fin = clock();//Fin de conteo de tiempo
        res.tSegudosCopia = (double)(fin - inicio) / CLOCKS_PER_SEC; //Calculo de tiempo transcurrido en segundos
        if (res.tSegudosCopia > 0)
            res.velocidadCopia = res.reg.bytesTotal / res.tSegudosCopia;

        if (!error_int && res.reg.bytesTotal == Torigen) {
            copiaExitosa = 1;
        } else {
            fprintf(stderr, "[REINTENTO %d/%d - syscall] bytes escritos: %ld, esperados: %ld\n",
                intentos, MAX_INTENTOS, res.reg.bytesTotal, Torigen);
            res.reg.errores++;
        }
    }

    close(fd_origen);

    //Se excedieron los intentos permitidos
    if (!copiaExitosa)
        fprintf(stderr, "[FALLO FINAL - syscall] No se pudo completar la copia tras %d intentos.\n", MAX_INTENTOS);

    return res;
}

// Copia por libreria
ResultadoCopia copia_libreria(const char *origen, const char *destino) {
    ResultadoCopia res = {0};
    int intento = 0;
    int copia_exitosa = 0;

    // Abrir origen
    FILE *f_origen = fopen(origen, "rb");
    if (f_origen == NULL) {
        manejar_errno("fopen origen [libreria]");
        res.reg.errores++;
        return res;
    }

    fseek(f_origen, 0, SEEK_END);
    long Torigen = ftell(f_origen);

    // Ciclo para repetir hasta cumplir con maximo de intentos o lograr una copia exitosa
    while (intento < MAX_INTENTOS && !copia_exitosa) {
        intento++;
        res.reg.buffLeidos  = 0;
        res.reg.buffEscritos = 0;
        res.reg.bytesTotal  = 0;
        rewind(f_origen);

        //Abrir destino
        FILE *f_destino = fopen(destino, "wb");
        if (f_destino == NULL) {
            manejar_errno("fopen destino [libreria]");
            res.reg.errores++;
            continue;
        }

        clock_t inicio  = clock();//Inicializacion de conteo de tiempo
        char buffer[BUFFER_SIZE];
        size_t  bytes_leidos;
        int error_int = 0;

        // Bucle de lectura/escritura
        while ((bytes_leidos = fread(buffer, 1, BUFFER_SIZE, f_origen)) > 0) {
            res.reg.buffLeidos++;

            size_t bytes_escritos = fwrite(buffer, 1, bytes_leidos, f_destino);
            if (bytes_escritos < bytes_leidos) {

                //Manejo de errores comunes en fwrite
                if (ferror(f_destino)) {
                    // Inferir causa más probable según errno del sistema
                    if (errno == ENOSPC) {
                        manejar_errno("fwrite disco lleno [libreria]");
                    } else if (errno == EACCES) {
                        manejar_errno("fwrite permiso denegado [libreria]");
                    } else {
                        manejar_errno("fwrite [libreria]");
                    }
                }
                res.reg.errores++;
                error_int = 1;
                break;
            }

            res.reg.buffEscritos++;
            res.reg.bytesTotal += bytes_escritos;
        }

        // Error en la lectura
        if (ferror(f_origen)) {
            manejar_errno("fread [libreria]");
            res.reg.errores++;
            error_int = 1;
        }

        clock_t fin = clock();//Fin de conteo de tiempo
        res.tSegudosCopia = (double)(fin - inicio) / CLOCKS_PER_SEC;//Calculo de tiempo transcurrido en segundos
        if (res.tSegudosCopia > 0)
            res.velocidadCopia = res.reg.bytesTotal / res.tSegudosCopia;

        fclose(f_destino);

        if (!error_int && res.reg.bytesTotal == Torigen) {
            copia_exitosa = 1;
        } else {
            fprintf(stderr, "[REINTENTO %d/%d - libreria] bytes escritos: %ld, esperados: %ld\n",
                intento, MAX_INTENTOS, res.reg.bytesTotal, Torigen);
            res.reg.errores++;
        }
    }

    fclose(f_origen);

    if (!copia_exitosa)
        fprintf(stderr, "[FALLO FINAL - libreria] No se pudo completar la copia tras %d intentos.\n", MAX_INTENTOS);

    return res;
}