#include "utilidades.h"
#include<stdio.h>


void limpiar_cola(ColaSolicitudes* cola){
    // Bloquear mutex para asegurar exclusión durante la limpieza
    sem_wait(&cola->mutex_cola);
    
    // Reiniciar índices de la cola circular
    cola->frente = 0;
    cola->final = 0; 
    cola->count = 0;
    
   
    
    // Reiniciar semáforo 'lleno' a MAX_SOLICITUDES_DIA
    // Primero consumimos todos los posts pendientes
    int valor_actual;
    sem_getvalue(&cola->lleno, &valor_actual);
    for(int i = 0; i < valor_actual; i++) {
        sem_trywait(&cola->lleno);  // Consume sin bloquear
    }
    // Ahora lleno está en 0, lo llevamos a MAX_SOLICITUDES_DIA
    for(int i = 0; i < MAX_SOLICITUDES_DIA; i++) {
        sem_post(&cola->lleno);
    }
    
    // Reiniciar semáforo 'vacio' a 0
    sem_getvalue(&cola->vacio, &valor_actual);
    for(int i = 0; i < valor_actual; i++) {
        sem_trywait(&cola->vacio);  // Consume todos los posts
    }
    // vacio ahora está en 0 (valor inicial correcto)
    
    // Liberar mutex
    sem_post(&cola->mutex_cola);
    
    printf("[SISTEMA] Cola limpiada correctamente\n");
}

//liberar nodos,paara contrl del dia
void liberar_nodos(SistemaEcoFlow* sistema){

     for(int i=0; i<TOTAL_HORAS; i++){

            sem_wait(&sistema->bloques[i].escritor);
            for(int h=0; h<NUM_NODOS; h++){
                 
                //liberar nodo fisico
                sem_wait(&sistema->nodos[h].mutex_nodo);

                sistema->nodos[h].reservado = 0;
                sistema->nodos[h].usuario_actual = -1;
                sistema->nodos[h].tipo_User = -1;
                sistema->nodos[h].hora_reserva = -1;
                sistema->nodos[h].ultima_modificacion = time(NULL);
                sem_post(&sistema->nodos[h].mutex_nodo);     
                
                //liberar bloques
                sistema->bloques[i].usuarios_en_nodo[h] = -1;
            }
            sem_post(&sistema->bloques[i].escritor);
        }
}

//liberar consumo, para control del dia
void liberar_consumo(SistemaEcoFlow* sistema){

     
    for(int h=0; h<NUM_NODOS; h++){
         
        sem_wait(&sistema->nodos[h].mutex_consumo);

        sistema->nodos[h].consumo_total_dia = 0;

        sem_post(&sistema->nodos[h].mutex_consumo);
    }
}

// GENERAR_PROBABILIDAD
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

// LIMPIAR_SISTEMA
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

// GENERAR_REPORTE_MENSUAL
void generar_reporte_mensual(SistemaEcoFlow* sistema) {
    printf("\n==============================================\n");
    printf("   REPORTE MENSUAL - ECO-FLOW 2026\n");
    printf("==============================================\n");
    printf("M3 Totales Procesados:    %.2f\n", (sistema->total_metros_cubicos)/1000); 
    printf("Amonestaciones Emitidas:  %d\n", sistema->total_amonestaciones); 
    printf("Consumos Críticos (Industria/Emerge): %d\n", sistema->total_consumos_criticos);
    printf("Consumos Estándar (Residencial):      %d\n", sistema->total_consumos_estandar);
    printf("Consumo criticos validados: %d\n",sistema->auditor.consumos_criticos_validados);
    printf("Consumo no justificados: %d\n",sistema->auditor.consumos_criticos_no_justificados);
    printf("Multas generadas: %d\n",sistema->auditor.multas_generadas);

    
    float total = (float)sistema->total_reservas_exitosas + sistema->total_reservas_fallidas;
    float eficiencia = (total > 0) ? ((float)sistema->total_reservas_exitosas / total) * 100 : 0;
    
    printf("Eficiencia Operativa:     %.2f%%\n", eficiencia);
    printf("==============================================\n");
}

// VALIDAR_HORA
void validar_hora(int hora){

     if(hora < HORA_INICIO || hora > HORA_FIN){

        printf("Hora %d:00 fuera de horario\n",hora);
        return;

    }
}

const char *accion_a_string(TipoAccion accion){

    switch (accion){

    case ACCION_RESERVA: return "RESERVA";
    case ACCION_CONSUMO: return "CONSUMO";
    case ACCION_CANCELACION: return "CANCELACION";
    case ACCION_CONSULTA: return "CONSULTA";
    case ACCION_PAGO: return "PAGO";
        
    break;
    
    default: return "DESCONOCIDO";
    break;
    }
}
