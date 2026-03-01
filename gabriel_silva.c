#include<stdio.h>
#include "structuras.h"


void inicializar_nodos(SistemaEcoFlow* sistema){

    for(int i=0; i<NUM_NODOS; i++){

        sistema->nodos[i].id_nodo = i;
        sistema->nodos[i].reservado = 0;
        sistema->nodos[i].usuario_actual = -1;
        sistema->nodos[i].hora_reserva = 0;
        sistema->nodos[i].consumo_total_dia = 0;
        sistema->nodos[i].consumo_total_mes = 0;
        sistema->nodos[i].ultima_modificacion = time(NULL);

        sem_init(&sistema->nodos[i].mutex_nodo,0,1);  // activo al comienzo
        sem_init(&sistema->nodos[i].mutex_consumo,0,1); //activo al comienzo

    }
}



void inicializar_bloques(SistemaEcoFlow* sistema){

   for(int i=0; i<TOTAL_HORAS; i++){

        sistema->bloques[i].hora = HORA_INICIO + i;
        // NOTAA elimine el arreglo nodos_disponibles dentro de bloque horario, nos basta con Usuarios_en_nodo
        //marcar todos los nodos disponibles dentro de cada bloque horario
        for(int h=0; h<NUM_NODOS; h++){

            sistema->bloques[i].usuarios_en_nodo[h] = -1;

        }

        sistema->bloques[i].lectores_activos = 0;
        sistema->bloques[i].reservas_realizadas = 0;
        sistema->bloques[i].consultas_realizadas = 0;

        sem_init(&sistema->bloques[i].mutex_lectores,0,1); //activo al inicio
        sem_init(&sistema->bloques[i].escritor,0,1); //activo al inicio
    } 

}


void inicializar_cola(ColaSolicitudes* cola){

    cola->frente = 0;
    cola-> final = 0;
    cola-> count = 0;

    //NOTA. no inicialice el arreglo solicitudes, solo se reserva como un espacio de memoria, se inicia cada pocicion en proceso hilo usuario

    sem_init(&cola->mutex_cola,0,1);

    //semaforos para manejo de productor consumidor, con bufer acotado
    sem_init(&cola->lleno,0,MAX_SOLICITUDES_DIA); //todos los espacios disponible
    sem_init(&cola->vacio,0,0); //ningun elemento
}


void inicializar_sistema(SistemaEcoFlow* sistema){

    //inicializar auditor
    sistema->auditor.consumos_criticos_no_justificados = 0;
    sistema->auditor.consumos_criticos_validados = 0;
    sistema->auditor.multas_generadas = 0;
    sistema->auditor.eficiencia_global = 0.0;
    sistema->auditor.nodos_ocupados_promedio = 0;
    sistema->auditor.tiempo_espera_promedio = 0;
    sem_init(&sistema->auditor.mutex_auditoria,0,0);

    //inicializar monitor
    for(int i=0; i<NUM_NODOS; i++){

        sistema->monitor.presion_actual[i] = 50 + rand() % 50;
        sistema->monitor.caudal_actual[i] = 0;
        
    }

    sistema->monitor.alertas_activas = 0;
    //memset(); // terminar limpiado de dashboard, no recuerdo jajaja
    sem_init(&sistema->monitor.mutex_monitor,0,1);


    //inicializar el resto de variables de sistema


}


//procedimiento utilizado para limpiar la cola de solicitudes al final de cada dia.
void limpiar_cola(ColaSolicitudes* cola){

    cola->frente = 0;
    cola->final = 0; 
    cola->count = 0;

    //volver a activar el semaforo de exclusion mutua
    sem_post(&cola->mutex_cola);

    //destruir semaforos de productoor consumidor
    sem_destroy(&cola->lleno);
    sem_destroy(&cola->vacio);

    //crear de nuevo los semaforos, creo que es mejor asi que intentar limpiarllos
    sem_init(&cola->lleno,0,MAX_SOLICITUDES_DIA);
    sem_init(&cola->vacio,0,0);
}



//esto ya es un procedimiento, no inicializacion
void generar_reporte_mensual(SistemaEcoFlow* sistema);


