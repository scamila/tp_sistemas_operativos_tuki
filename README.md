# TP SISTEMAS OPERATIVOS 2023 - 1° CUATRIMESTRE
The Ultimate Kernel Implementation (T.U.K.I.)

El trabajo práctico consiste en desarrollar una solución que permita la simulación de un sistema distribuido,el cual consiste de 5 módulos: Consola (múltiples instancias), Kernel, CPU, Memoria y File System (1 instancia cada uno).

El proceso de ejecución del mismo consiste en crear procesos a través del módulo Consola, los cuales enviarán la información necesaria al módulo Kernel para que el mismo pueda crear las estructuras necesarias a fin de administrar y planificar su ejecución mediante diversos algoritmos.

Estos procesos serán ejecutados en el módulo CPU, quien interpretará sus instrucciones y hará las peticiones necesarias a Memoria y/o al Kernel.

La Memoria administrará el espacio de memoria (valga la redundancia) de estos procesos implementando un esquema de segmentación y respondiendo a las peticiones de CPU, Kernel y File System.

El File System implementará un esquema indexado, tomando algunas características de un File System tipo Unix o ext2. El mismo estará encargado de administrar y persistir los archivos creados por los procesos que corren en el sistema, respondiendo a las peticiones de Kernel y haciendo las peticiones necesarias a Memoria.

Una vez que un proceso finalice tras haber sido ejecutadas todas sus instrucciones, el Kernel devolverá un mensaje de finalización a su Consola correspondiente y cerrará la conexión.


# Protocolo de Comunicación

![alt text](https://github.com/scamila/tp_sistemas_operativos_tuki/blob/main/resources/protocolo%20de%20comunicacion%20SSOO.drawio.png)
