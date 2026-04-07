#include "smart_copy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Verificacion del archivo
int verificar_archivo(const char *nombre) {
    FILE *f = fopen(nombre, "rb");
    if (f == NULL) {
        printf("  [ERROR] No se encontro: %s\n", nombre);
        printf("  Asegurate de tener el archivo en la misma carpeta.\n");
        return 0;
    }
    fclose(f);
    return 1;
}

// Archivo temporal para poder realizar pruebas sin afectar archivos reales
int generar_archivo_simulado(const char *ruta, long bytes) {
    FILE *f = fopen(ruta, "wb");
    if (!f) {
        perror("Error creando archivo simulado");
        return 0;
    }

    char bloque[BUFFER_SIZE];
    memset(bloque, 'A', sizeof(bloque));

    long escritos = 0;
    while (escritos < bytes) {
        long restante   = bytes - escritos;
        long a_escribir = restante < 4096 ? restante : 4096;
        fwrite(bloque, 1, a_escribir, f);
        escritos += a_escribir;
    }

    fclose(f);
    return 1;
}

// Estructura de cada fila de la tabla
typedef struct {
    char etiqueta[32];
    ResultadoCopia sys;
    ResultadoCopia lib;
} FilaResultado;

// Recoleccion de datos para cada prueba
int recolectar_prueba(const char *etiqueta, long bytes, FilaResultado *fila) {
    const char *tmp_origen  = "sim_origen.tmp";
    const char *tmp_syscall = "sim_dest_syscall.tmp";
    const char *tmp_lib     = "sim_dest_lib.tmp";

    strncpy(fila->etiqueta, etiqueta, sizeof(fila->etiqueta) - 1);
    fila->etiqueta[sizeof(fila->etiqueta) - 1] = '\0';

    printf("  Procesando %s... ", etiqueta);
    fflush(stdout);

    if (!generar_archivo_simulado(tmp_origen, bytes)) return 0;

    fila->sys = copia_buffer  (tmp_origen, tmp_syscall);
    fila->lib = copia_libreria(tmp_origen, tmp_lib);

    remove(tmp_origen);
    remove(tmp_syscall);
    remove(tmp_lib);

    printf("listo.\n");
    return 1;
}

// Impresion de los datos en formato de tabla
void imprimir_tabla(FilaResultado *filas, int n) {
    printf("\n");
    printf("=============================================================================\n");
    printf("                         TABLA COMPARATIVA                                  \n");
    printf("=============================================================================\n");
    printf("| %-6s | %-29s | %-29s |\n",
           "", "  SYSCALL (llamada directa)", "    LIBRERIA (buffered)   ");
    printf("| %-6s | %-13s | %-13s | %-13s | %-13s |\n",
           "Tamano", "   Tiempo    ", "  Velocidad  ", "   Tiempo    ", "  Velocidad  ");
    printf("|--------|---------------|---------------|---------------|---------------|\n");

    for (int i = 0; i < n; i++) {
        double vel_sys = (filas[i].sys.tSegudosCopia > 0.0)
                         ? filas[i].sys.velocidadCopia / (1024.0 * 1024.0) : 0.0;
        double vel_lib = (filas[i].lib.tSegudosCopia > 0.0)
                         ? filas[i].lib.velocidadCopia / (1024.0 * 1024.0) : 0.0;

        printf("| %-6s | %9.6f s  | %9.2f MB/s | %9.6f s  | %9.2f MB/s |\n",
               filas[i].etiqueta,
               filas[i].sys.tSegudosCopia, vel_sys,
               filas[i].lib.tSegudosCopia, vel_lib);
    }

    printf("|--------|---------------|---------------|---------------|---------------|\n");
    printf("=============================================================================\n");
}

// ─── Main ─────────────────────────────────────────────────────────────────
int main(int argc, char *argv[]) {
    printf("BACKUP EAFITOS \n");


    // Escenario 1: el usuario guarda su propio archivo
    if (argc == 3) {
        printf("\nArchivo proporcionado por el usuario\n");
        printf("Origen:  %s\n", argv[1]);
        printf("Destino: %s\n\n", argv[2]);

        if (!verificar_archivo(argv[1]))
            return 1;

        printf("Ejecutando copia...\n\n");

        ResultadoCopia r1 = copia_buffer  (argv[1], argv[2]);
        ResultadoCopia r2 = copia_libreria(argv[1], argv[2]);

        FilaResultado fila;
        
        long bytes = (long)(r1.velocidadCopia * r1.tSegudosCopia);

        if (bytes < 1024){
            snprintf(fila.etiqueta, sizeof(fila.etiqueta), "%.10ld B", bytes);
        } else if (bytes < 1024 * 1024){
            snprintf(fila.etiqueta, sizeof(fila.etiqueta), "%.10ld KB", bytes / 1024);
        }else if (bytes < 1024L * 1024 * 1024){
            snprintf(fila.etiqueta, sizeof(fila.etiqueta), "%.10ld MB", bytes / (1024 * 1024));
        }else{
            snprintf(fila.etiqueta, sizeof(fila.etiqueta), "%.10ld GB", bytes / (1024L * 1024 * 1024));
        }
        fila.sys = r1;
        fila.lib = r2;
        imprimir_tabla(&fila, 1);
        
        return 0;
    }

    // Escenario 2: tabla comparativa con archivos SIMULADOS
    printf("\nModo: tabla comparativa con archivos simulados\n");
    printf("Recolectando datos, por favor espere...\n\n");

    FilaResultado filas[3];

    if (!recolectar_prueba("1 KB",  1L * 1024,               &filas[0])) return 1;
    if (!recolectar_prueba("1 MB",  1L * 1024 * 1024,        &filas[1])) return 1;
    if (!recolectar_prueba("1 GB",  1L * 1024 * 1024 * 1024, &filas[2])) return 1;

    // La tabla se imprime SOLO cuando todos los datos están listos
    imprimir_tabla(filas, 3);

    printf("\nPruebas finalizadas.\n");
    printf("=============================================================================\n");

    return 0;
}