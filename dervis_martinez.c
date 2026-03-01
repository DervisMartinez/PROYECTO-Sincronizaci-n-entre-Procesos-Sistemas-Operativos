#include <stdio.h>
#include "structuras.h"
#include <string.h>


void inicializar_nodos(SistemaEcoFlow* sistema){
 for (int i = 0; i < NUM_NODOS; i++) {//  Itera sobre cada nodo, desde i = 0 hasta i = 9 (porque NUM_NODOS = 10).
        sistema->nodos[i].id_nodo = i;
        sistema->nodos[i].reservado = 0;
        sistema->nodos[i].usuario_actual = -1;
        sistema->nodos[i].hora_reserva = -1;
        sistema->nodos[i].consumo_total_dia = 0;
        sistema->nodos[i].consumo_total_mes = 0;
        sistema->nodos[i].veces_reservado = 0;
        sistema->nodos[i].ultima_modificacion = time(NULL);
        
        sem_init(&sistema->nodos[i].mutex_nodo, 0, 1);
        sem_init(&sistema->nodos[i].mutex_consumo, 0, 1);
    }
}

void terminar_lectura(BloqueHorario* bloque) {
    sem_wait(&bloque->mutex_lectores);
    bloque->lectores_activos--;
    if (bloque->lectores_activos == 0) {
        sem_post(&bloque->escritor);
    }
    sem_post(&bloque->mutex_lectores);
}

void iniciar_escritura(BloqueHorario* bloque) {
    sem_wait(&bloque->escritor);
}

void terminar_escritura(BloqueHorario* bloque) {
    sem_post(&bloque->escritor);
}

// ==================== FUNCIONES DE USUARIO ====================

int reservar_nodo(SistemaEcoFlow* sistema, int id_usuario, int hora, int tipo_usuario) {
    if (hora < HORA_INICIO || hora >= HORA_FIN) {
        return -1;  // Fuera de horario
    }
    
    int idx_hora = hora - HORA_INICIO;
    BloqueHorario* bloque = &sistema->bloques[idx_hora];
    
    iniciar_escritura(bloque);  // Bloqueamos escritura (modificar disponibilidad)
    
    // Buscar nodo disponible
    int nodo_asignado = -1;
    for (int i = 0; i < NUM_NODOS; i++) {
        if (bloque->nodos_disponibles[i] == 1) {
            nodo_asignado = i;
            break;
        }
    }
    
    if (nodo_asignado != -1) {
        // Reservar el nodo
        bloque->nodos_disponibles[nodo_asignado] = 0;
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
    
    terminar_escritura(bloque);
    return nodo_asignado;
}

int cancelar_reserva(SistemaEcoFlow* sistema, int id_usuario) {
    int cancelado = 0;
    
    // Buscar en todos los bloques horarios
    for (int h = 0; h < TOTAL_HORAS; h++) {
        BloqueHorario* bloque = &sistema->bloques[h];
        
        iniciar_escritura(bloque);
        
        for (int i = 0; i < NUM_NODOS; i++) {
            if (bloque->usuarios_en_nodo[i] == id_usuario) {
                // Cancelar reserva
                bloque->nodos_disponibles[i] = 1;
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
                
                terminar_escritura(bloque);
                return 1;
            }
        }
        
        terminar_escritura(bloque);
    }
    
    // No encontró reserva -> amonestación
    registrar_amonestacion(sistema, id_usuario);
    printf(" [USUARIO %d] AMONESTACIÓN: Cancelación sin reserva activa\n", id_usuario);
    
    return 0;
}
