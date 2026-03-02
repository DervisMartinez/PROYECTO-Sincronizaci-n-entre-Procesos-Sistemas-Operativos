#include <stdio.h>
#include <stdlib.h>
#include "structuras.h"


// ==================== FUNCIONES DE USUARIO ====================


//RESERVAR NODO
int reservar_nodo(SistemaEcoFlow* sistema, int id_usuario, int hora, int tipo_usuario) {
    if (hora < HORA_INICIO || hora >= HORA_FIN) {
        return -1;  // Fuera de horario
    }
    
    int idx_hora = hora - HORA_INICIO;
    BloqueHorario* bloque = &sistema->bloques[idx_hora];
    
    sem_wait(bloque);  // Bloqueamos escritura (modificar disponibilidad)
    
    // Buscar nodo disponible
    int nodo_asignado = -1;
    for (int i = 0; i < NUM_NODOS; i++) {
        if (bloque->usuarios_en_nodo[i] == -1) {
            nodo_asignado = i;
            break;
        }
    }
    
    if (nodo_asignado != -1) {
        // Reservar el nodo
        bloque->usuarios_en_nodo[nodo_asignado] = 0;
        bloque->usuarios_en_nodo[nodo_asignado] = id_usuario;
        bloque->reservas_realizadas++;
        
        // Actualizar estadísticas del nodo
        sem_wait(&sistema->nodos[nodo_asignado].mutex_nodo);
        sistema->nodos[nodo_asignado].reservado = 1;
        sistema->nodos[nodo_asignado].usuario_actual = id_usuario;
        sistema->nodos[nodo_asignado].hora_reserva = hora;
        sistema->nodos[nodo_asignado].veces_reservado++;
        sistema->nodos[nodo_asignado].ultima_modificacion = time(NULL);
        sem_post(&sistema->nodos[nodo_asignado].mutex_nodo);
        
        // Actualizar estadísticas globales
        sem_wait(&sistema->mutex_estadisticas);
        sistema->total_reservas_exitosas++;
        sem_post(&sistema->mutex_estadisticas);
        
        printf("[USUARIO %d] Nodo %d reservado para las %d:00\n", 
               id_usuario, nodo_asignado, hora);
    } else {
        sem_wait(&sistema->mutex_estadisticas);
        sistema->total_reservas_fallidas++;
        sem_post(&sistema->mutex_estadisticas);
        
        printf(" [USUARIO %d] No hay nodos disponibles para las %d:00\n", 
               id_usuario, hora);
    }
    
    sem_post(bloque);
    return nodo_asignado;
}

//CANCELAR RESERVA

int cancelar_reserva(SistemaEcoFlow* sistema, int id_usuario) {
    int cancelado = 0;
    
    // Buscar en todos los bloques horarios
    for (int h = 0; h < TOTAL_HORAS; h++) {
        BloqueHorario* bloque = &sistema->bloques[h];
        
        sem_wait(bloque);
        
        for (int i = 0; i < NUM_NODOS; i++) {
            if (bloque->usuarios_en_nodo[i] == id_usuario) {
                // Cancelar reserva
                bloque->usuarios_en_nodo[i] = 1;
                bloque->usuarios_en_nodo[i] = -1;
                cancelado = 1;
                
                // Actualizar nodo
                sem_wait(&sistema->nodos[i].mutex_nodo);
                sistema->nodos[i].reservado = 0;
                sistema->nodos[i].usuario_actual = -1;
                sistema->nodos[i].hora_reserva = -1;
                sistema->nodos[i].ultima_modificacion = time(NULL);
                sem_post(&sistema->nodos[i].mutex_nodo);
                
                printf(" [USUARIO %d] Canceló reserva del nodo %d a las %d:00\n", 
                       id_usuario, i, bloque->hora);
                
                sem_post(bloque);
                return 1;
            }
        }
        
        sem_post(bloque);
    }
    
    // No encontró reserva -> amonestación
    registrar_amonestacion(sistema, id_usuario);
    printf(" [USUARIO %d] AMONESTACIÓN: Cancelación sin reserva activa\n", id_usuario);
    
    return 0;
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
