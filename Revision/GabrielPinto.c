#include <stdio.h>
#include <string.h>
#include <time.h>
#include "structuras.h"

/* * ============================================================================
 * FASE 1: INICIALIZACIÓN Y GESTIÓN DE RECURSOS
 * ============================================================================
 */

void inicializar_nodos(SistemaEcoFlow* sistema) {
    for (int i = 0; i < NUM_NODOS; i++) {
        sistema->nodos[i].id_nodo = i;
        sistema->nodos[i].reservado = 0;
        sistema->nodos[i].usuario_actual = -1;
        sistema->nodos[i].hora_reserva = -1;
        sistema->nodos[i].consumo_total_dia = 0;
        sistema->nodos[i].consumo_total_mes = 0;
        sistema->nodos[i].veces_reservado = 0;
        sistema->nodos[i].ultima_modificacion = time(NULL);
        
        // Mutex para proteger cambios de estado y registro de consumos
        sem_init(&sistema->nodos[i].mutex_nodo, 0, 1);
        sem_init(&sistema->nodos[i].mutex_consumo, 0, 1);
    }
}


void limpiar_sistema(SistemaEcoFlow* sistema) {
    // Destrucción de semáforos de nodos
    for (int i = 0; i < NUM_NODOS; i++) {
        sem_destroy(&sistema->nodos[i].mutex_nodo);
        sem_destroy(&sistema->nodos[i].mutex_consumo);
    }
    
    // Destrucción de semáforos de bloques horarios
    for (int h = 0; h < TOTAL_HORAS; h++) {
        sem_destroy(&sistema->bloques[h].mutex_lectores);
        sem_destroy(&sistema->bloques[h].escritor);
    }
    
    // Sincronización de cola (Productor-Consumidor)
    sem_destroy(&sistema->cola_solicitudes.mutex_cola);
    sem_destroy(&sistema->cola_solicitudes.lleno);
    sem_destroy(&sistema->cola_solicitudes.vacio);
    
    // Semáforos de estadísticas y monitoreo
    sem_destroy(&sistema->mutex_global);
    sem_destroy(&sistema->mutex_estadisticas);
    sem_destroy(&sistema->mutex_dia);
    sem_destroy(&sistema->auditor.mutex_auditoria);
    sem_destroy(&sistema->monitor.mutex_monitor);
}


/* * ============================================================================
 * FASE 2: OPERACIONES DE USUARIO (RESERVAS Y CANCELACIONES)
 * ============================================================================
 */


int reservar_nodo(SistemaEcoFlow* sistema, int id_usuario, int hora, int tipo_usuario) {
    if (hora < HORA_INICIO || hora >= HORA_FIN) return -1; 
    
    int idx_hora = hora - HORA_INICIO;
    BloqueHorario* bloque = &sistema->bloques[idx_hora];
    
    // SECCIÓN CRÍTICA: Acceso exclusivo para modificar disponibilidad
    sem_wait(&bloque->escritor); 
    
    int nodo_asignado = -1;
    for (int i = 0; i < NUM_NODOS; i++) {
        if (bloque->usuarios_en_nodo[i] == -1) {
            nodo_asignado = i;
            break;
        }
    }
    
    if (nodo_asignado != -1) {
        // Actualizar estado en el bloque horario
        bloque->usuarios_en_nodo[nodo_asignado] = id_usuario;
        bloque->reservas_realizadas++;
        
        // Sincronizar actualización en la estructura del nodo específico
        sem_wait(&sistema->nodos[nodo_asignado].mutex_nodo);
        sistema->nodos[nodo_asignado].reservado = 1;
        sistema->nodos[nodo_asignado].usuario_actual = id_usuario;
        sistema->nodos[nodo_asignado].hora_reserva = hora;
        sistema->nodos[nodo_asignado].veces_reservado++;
        sistema->nodos[nodo_asignado].ultima_modificacion = time(NULL);
        sem_post(&sistema->nodos[nodo_asignado].mutex_nodo);
        
        // Actualizar métricas globales (Exclusión mutua)
        sem_wait(&sistema->mutex_estadisticas);
        sistema->total_reservas_exitosas++;
        sem_post(&sistema->mutex_estadisticas);
        
        printf("[SISTEMA] Usuario %d -> Reserva Nodo %d (%d:00)\n", id_usuario, nodo_asignado, hora);
    } else {
        sem_wait(&sistema->mutex_estadisticas);
        sistema->total_reservas_fallidas++;
        sem_post(&sistema->mutex_estadisticas);
        printf("[SISTEMA] Sin disponibilidad para Usuario %d a las %d:00\n", id_usuario, hora);
    }
    
    sem_post(&bloque->escritor);
    return nodo_asignado;
}


int cancelar_reserva(SistemaEcoFlow* sistema, int id_usuario) {
    for (int h = 0; h < TOTAL_HORAS; h++) {
        BloqueHorario* bloque = &sistema->bloques[h];
        
        sem_wait(&bloque->escritor);
        
        for (int i = 0; i < NUM_NODOS; i++) {
            if (bloque->usuarios_en_nodo[i] == id_usuario) {
                // Liberar en el bloque horario
                bloque->usuarios_en_nodo[i] = -1;
                
                // Liberar en la estructura del nodo
                sem_wait(&sistema->nodos[i].mutex_nodo);
                sistema->nodos[i].reservado = 0;
                sistema->nodos[i].usuario_actual = -1;
                sistema->nodos[i].hora_reserva = -1;
                sistema->nodos[i].ultima_modificacion = time(NULL);
                sem_post(&sistema->nodos[i].mutex_nodo);
                
                printf("[SISTEMA] Usuario %d -> Cancelación exitosa Nodo %d\n", id_usuario, i);
                sem_post(&bloque->escritor);
                return 1;
            }
        }
        sem_post(&bloque->escritor);
    }
    
    // Lógica de penalización (Fuera de sección crítica de bloques)
    registrar_amonestacion(sistema, id_usuario);
    printf("[ALERTA] Usuario %d -> Amonestado por cancelación inválida\n", id_usuario);
    return 0;
}

/* * ============================================================================
 * FASE 3: MONITOREO Y FINALIZACIÓN
 * ============================================================================
 */

void terminar_lectura(BloqueHorario* bloque) {
    sem_wait(&bloque->mutex_lectores);
    bloque->lectores_activos--;
    if (bloque->lectores_activos == 0) {
        sem_post(&bloque->escritor); // Libera el paso a escritores si no hay más lectores
    }
    sem_post(&bloque->mutex_lectores);
}


void generar_reporte_mensual(SistemaEcoFlow* sistema) {
    printf("\n==============================================\n");
    printf("   REPORTE MENSUAL - ECO-FLOW 2026\n");
    printf("==============================================\n");
    printf("M3 Totales Procesados:    %d\n", sistema->total_metros_cubicos); 
    printf("Amonestaciones Emitidas:  %d\n", sistema->total_amonestaciones); 
    printf("Consumos Críticos (Industria/Emerge): %d\n", sistema->total_consumos_criticos);
    printf("Consumos Estándar (Residencial):      %d\n", sistema->total_consumos_estandar);
    
    float total = (float)sistema->total_reservas_exitosas + sistema->total_reservas_fallidas;
    float eficiencia = (total > 0) ? ((float)sistema->total_reservas_exitosas / total) * 100 : 0;
    
    printf("Eficiencia Operativa:     %.2f%%\n", eficiencia);
    printf("==============================================\n");
}
