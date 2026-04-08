# Smart Backup Kernel-Space Utility

Proyecto académico de **Sistemas Operativos** que implementa una
herramienta de respaldo de archivos comparando dos métodos de copia:

-   Llamadas al sistema (syscalls)
-   Librerías estándar de C

## Objetivo

Analizar las diferencias de rendimiento entre operaciones realizadas con
llamadas al sistema y funciones de librería estándar.

## Estructura del Proyecto

    smart_copy.h        # Definición de funciones y constantes
    backup_engine.c     # Lógica de copia de archivos
    main.c              # Programa principal
    Makefile            # Compilación automática
    pruebas/            # Archivos de prueba

## Compilación

``` bash
gcc main.c backup_engine.c -o smart_backup
```

## Ejecución

``` bash
./smart_backup archivo_origen archivo_destino
```

Ejemplo:

``` bash
./smart_backup prueba.txt copia.txt
```

## Funcionamiento

El programa realiza dos tipos de copia:

1.  **Syscall copy**
    -   open()
    -   read()
    -   write()
2.  **Library copy**
    -   fopen()
    -   fread()
    -   fwrite()

Luego mide el tiempo de ejecución de cada método y muestra una tabla
comparativa.

## Manejo de errores

El sistema detecta:

-   Archivos inexistentes
-   Permisos insuficientes
-   Fallos de lectura o escritura

En caso de error, el sistema intenta la operación hasta **3 veces**
antes de fallar.

## Resultados

El programa muestra una tabla comparativa con:

-   Tamaño del archivo
-   Tiempo de copia
-   Velocidad de transferencia

## Autores

Alessandro Soccol Mejía

Ismael García Ceballos

## About

Group assignment — Operating Systems (Sistemas Operativos), EAFIT 2026-1.
Base code reference: [evalenciEAFIT/courses](https://github.com/evalenciEAFIT/courses/tree/main/SO_XV6/tema/backup)