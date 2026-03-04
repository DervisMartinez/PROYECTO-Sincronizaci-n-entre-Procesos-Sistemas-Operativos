#include <structuras.h>
#include "structuras.h"

// Implementación de la Regla de Probabilidad (50% Reserva)
void decidir_accion_usuario(Solicitud* solicitud) {
    int r = rand() % 100;
    
    if (r < 50) { // 50% de probabilidad para reserva según la Ley
        solicitud->accion = ACCION_RESERVA;
    } else if (r < 70) {
        solicitud->accion = ACCION_CONSULTA; // Consulta de presión (Lectores)
    } else if (r < 85) {
        solicitud->accion = ACCION_CONSUMO;  // Acción de "trabajar"
    } else if (r < 95) {
        solicitud->accion = ACCION_CANCELACION;
    } else {
        solicitud->accion = ACCION_PAGO;
    }
}

// Implementación de la Regla 2: Lectores/Escritores (Consulta de Presión)
void consultar_presion_nodo(SistemaEcoFlow* sistema, int hora) {
    BloqueHorario* bloque = &sistema->bloques[hora - HORA_INICIO];

    // Entrada del Lector
    sem_wait(&bloque->mutex_lectores);
    bloque->lectores_activos++;
    if (bloque->lectores_activos == 1) {
        sem_wait(&bloque->escritor); // El primer lector bloquea a los escritores
    }
    sem_post(&bloque->mutex_lectores);

    // SECCIÓN CRÍTICA DE LECTURA (Simultánea)
    printf("[LECTOR] Usuario consultando presión en bloque %d:00... OK\n", hora);
    usleep(100000); // Simula el tiempo de la comunicación inalámbrica

    // Salida del Lector
    sem_wait(&bloque->mutex_lectores);
    bloque->lectores_activos--;
    if (bloque->lectores_activos == 0) {
        sem_post(&bloque->escritor); // El último lector libera a los escritores
    }
    sem_post(&bloque->mutex_lectores);
}

void procesar_consumo_hídrico(SistemaEcoFlow* sistema, Solicitud* sol) {
    int litros = rand() % 800; // Genera consumo aleatorio
    
    if (litros > CONSUMO_CRITICO_LIMITE) {
        sem_wait(&sistema->auditor.mutex_auditoria);
        
        if (sol->tipo_usuario == 1) { // Industrial: Justificado por industria crítica
            sistema->total_consumos_criticos++;
            printf("[AUDITOR] Consumo crítico JUSTIFICADO (Industrial): %d L\n", litros);
        } else { // Residencial: Sin justificación = multa/amonestación
            sistema->total_amonestaciones++;
            printf("[AUDITOR] ALERTA: Consumo crítico NO justificado (Residencial): %d L\n", litros);
        }
        
        sem_post(&sistema->auditor.mutex_auditoria);
    } else {
        sistema->total_consumos_estandar++;
    }
    
    sistema->total_metros_cubicos += litros;
}

#include <structuras.h>
#include "structuras.h"

// Implementación de la Regla de Probabilidad (50% Reserva)
void decidir_accion_usuario(Solicitud* solicitud) {
    int r = rand() % 100;
    
    if (r < 50) { // 50% de probabilidad para reserva según la Ley
        solicitud->accion = ACCION_RESERVA;
    } else if (r < 70) {
        solicitud->accion = ACCION_CONSULTA; // Consulta de presión (Lectores)
    } else if (r < 85) {
        solicitud->accion = ACCION_CONSUMO;  // Acción de "trabajar"
    } else if (r < 95) {
        solicitud->accion = ACCION_CANCELACION;
    } else {
        solicitud->accion = ACCION_PAGO;
    }
}

// Implementación de la Regla 2: Lectores/Escritores (Consulta de Presión)
void consultar_presion_nodo(SistemaEcoFlow* sistema, int hora) {
    BloqueHorario* bloque = &sistema->bloques[hora - HORA_INICIO];

    // Entrada del Lector
    sem_wait(&bloque->mutex_lectores);
    bloque->lectores_activos++;
    if (bloque->lectores_activos == 1) {
        sem_wait(&bloque->escritor); // El primer lector bloquea a los escritores
    }
    sem_post(&bloque->mutex_lectores);

    // SECCIÓN CRÍTICA DE LECTURA (Simultánea)
    printf("[LECTOR] Usuario consultando presión en bloque %d:00... OK\n", hora);
    usleep(100000); // Simula el tiempo de la comunicación inalámbrica

    // Salida del Lector
    sem_wait(&bloque->mutex_lectores);
    bloque->lectores_activos--;
    if (bloque->lectores_activos == 0) {
        sem_post(&bloque->escritor); // El último lector libera a los escritores
    }
    sem_post(&bloque->mutex_lectores);
}

void procesar_consumo_hidrico(SistemaEcoFlow* sistema, Solicitud* sol) {
    int litros = rand() % 800; // Genera consumo aleatorio
    
    if (litros > CONSUMO_CRITICO_LIMITE) {
        sem_wait(&sistema->auditor.mutex_auditoria);
        
        if (sol->tipo_usuario == 1) { // Industrial: Justificado por industria crítica
            sistema->total_consumos_criticos++;
            printf("[AUDITOR] Consumo crítico JUSTIFICADO (Industrial): %d L\n", litros);
        } else { // Residencial: Sin justificación = multa/amonestación
            sistema->total_amonestaciones++;
            printf("[AUDITOR] ALERTA: Consumo crítico NO justificado (Residencial): %d L\n", litros);
        }
        
        sem_post(&sistema->auditor.mutex_auditoria);
    } else {
        sistema->total_consumos_estandar++;
    }
    
    sistema->total_metros_cubicos += litros;
}

