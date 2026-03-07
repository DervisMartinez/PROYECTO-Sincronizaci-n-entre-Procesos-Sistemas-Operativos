#ifndef PROCESO_HILO_H
#define PROCESO_HILO_H

#include <unistd.h>
#include <stdio.h>
#include "utilidades.h"
#include "structuras.h"
#include "user_process.h"

// ==================== PROCESADOR DE SOLICITUDES (CONSUMIDOR) ====================
void* procesar_solicitudes(void* arg) {
    SistemaEcoFlow* sistema = (SistemaEcoFlow*)arg;

    printf("\n✅ PROCESADOR DE SOLICITUDES INICIADO\n");
    printf("   Consumiendo solicitudes de la cola\n\n");

    while (sistema->simulacion_activa) {
        
        // Esperar a que haya una solicitud en la cola
        sem_wait(&sistema->cola_solicitudes.vacio);

        // Acceso exclusivo a la cola
        sem_wait(&sistema->cola_solicitudes.mutex_cola);

        // Verificar que la cola no esté vacía
        if (sistema->cola_solicitudes.count <= 0) {
            sem_post(&sistema->cola_solicitudes.mutex_cola);
            sem_post(&sistema->cola_solicitudes.lleno);
            continue;
        }

        // Extraer solicitud del frente de la cola
        Solicitud solicit = sistema->cola_solicitudes.solicitudes[sistema->cola_solicitudes.frente];

        // Actualizar frente de la cola (circular)
        sistema->cola_solicitudes.frente = (sistema->cola_solicitudes.frente + 1) % MAX_SOLICITUDES_DIA;
        sistema->cola_solicitudes.count--;

        sem_post(&sistema->cola_solicitudes.mutex_cola);

        // Liberar espacio en la cola
        sem_post(&sistema->cola_solicitudes.lleno);

        // Mostrar la solicitud a procesar
        printf("▶️ Procesando: Usuario %d - %s para %d:00\n", 
               solicit.id_usuario, 
               accion_a_string(solicit.accion), 
               solicit.hora_solicitada);

        // Ejecutar la acción del usuario
        switch (solicit.accion) {
            case ACCION_RESERVA:
                // ✅ CORREGIDO: Ahora pasa los 5 argumentos correctamente
                reservar_nodo(sistema, 
                             solicit.id_usuario, 
                             solicit.hora_solicitada, 
                             solicit.tipo_usuario,
                             solicit.nodo_preferido);
                break;
                
            case ACCION_CONSUMO:
                consumir_agua(sistema, 
                             solicit.id_usuario, 
                             solicit.nodo_preferido, 
                             solicit.hora_solicitada, 
                             solicit.litros_consumir, 
                             solicit.tipo_usuario);
                break;
                
            case ACCION_CANCELACION:
                cancelar_reserva(sistema, solicit.id_usuario);
                break;
                
            case ACCION_CONSULTA:
                consultar_presion(sistema, solicit.hora_solicitada);
                break;
                
            case ACCION_PAGO:
                pagar_excedente(sistema, 
                               solicit.id_usuario, 
                               solicit.hora_solicitada, 
                               solicit.nodo_preferido);
                break;
                
            default:
                printf("⚠️ Acción desconocida: %s\n", accion_a_string(solicit.accion));
                break;
        }

        usleep(50000); // 0.05 segundos
    }

    printf("\n🛑 Procesador de solicitudes finalizado\n");
    return NULL;
}

// ==================== MONITOR DE PRESIÓN ====================
void* proceso_monitor(void* arg) {
    SistemaEcoFlow* sistema = (SistemaEcoFlow*)arg;

    while (sistema->simulacion_activa) {
        sleep(10); // Mostrar cada 10 segundos

        // Bloqueo de sección crítica
        sem_wait(&sistema->monitor.mutex_monitor);

        printf("\n📊 MONITOR DE PRESIÓN - Día %d\n", sistema->dia_actual);
        printf("   %s\n", "==========================================");
        
        for (int i = 0; i < NUM_NODOS; i++) {
            printf("   NODO %2d: Presión %3d psi [%s]\n", 
                   i + 1, 
                   sistema->monitor.presion_actual[i],
                   sistema->nodos[i].reservado ? "🔴 OCUPADO" : "🟢 LIBRE");
        }

        // Estadísticas
        printf("\n   📈 Solicitudes: %3d/%-3d", 
               sistema->solicitudes_dia_actual, MAX_SOLICITUDES_DIA);
        printf("   |   Eficiencia: %.1f%%\n\n", sistema->auditor.eficiencia_global);

        // Liberación de recurso
        sem_post(&sistema->monitor.mutex_monitor);
    }
    return NULL;
}

// ==================== CONTROLADOR DE DÍA ====================
void* control_dia(void* arg) {
    SistemaEcoFlow* sistema = (SistemaEcoFlow*)arg;

    printf("\n📅 CONTROLADOR DE DÍA INICIADO\n");
    printf("   Simulando %d días\n\n", DIAS_SIMULACION);

    while (sistema->dia_actual <= DIAS_SIMULACION) {
        
        printf("\n🚀 DÍA %d - INICIO (6:00 AM)\n", sistema->dia_actual);
        sleep(12); // 12 segundos = 1 día de simulación
        printf("\n⏰ DÍA %d - FIN (6:00 PM)\n", sistema->dia_actual);

        // Esperar a que se procesen las solicitudes pendientes
        printf("   ⏳ Procesando últimas solicitudes...\n");
        int espera = 0;
        while (sistema->cola_solicitudes.count > 0 && espera < 30) {
            sleep(1);
            espera++;
        }

        // Limpiar cola para el nuevo día
        printf("   🧹 Limpiando cola de solicitudes...\n");
        limpiar_cola(&sistema->cola_solicitudes);

        // Reiniciar contador de solicitudes
        sistema->solicitudes_dia_actual = 0;

        // Liberar nodos
        printf("   🔓 Liberando nodos...\n");
        liberar_nodos(sistema);

        // Reiniciar consumo diario
        printf("   📉 Reiniciando consumo diario...\n");
        liberar_consumo(sistema);

        sistema->dia_actual++;

        if (sistema->dia_actual <= DIAS_SIMULACION) {
            printf("\n✅ Sistema listo para DÍA %d\n", sistema->dia_actual);
        }
    }

    printf("\n🏁 SIMULACIÓN COMPLETADA. %d días transcurridos\n", DIAS_SIMULACION);
    sistema->simulacion_activa = 0;

    return NULL;
}

// ==================== AUDITOR DE FLUJO ====================
void* proceso_auditor(void* arg) {
    SistemaEcoFlow* sistema = (SistemaEcoFlow*)arg;

    printf("\n🔍 AUDITOR DE FLUJO iniciado\n");
    printf("   Supervisando consumos críticos (>%dL)\n", CONSUMO_CRITICO_LIMITE);

    int consumos_por_hora[TOTAL_HORAS] = {0};

    while (sistema->simulacion_activa) {
        
        // Esperar notificación de consumo crítico (timeout 15 segundos)
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 15;

        int notificado = sem_timedwait(&sistema->auditor.mutex_auditoria, &ts);

        if (notificado == 0) {
            printf("\n🔔 AUDITOR: Validando consumos críticos pendientes...\n");

            for (int n = 0; n < NUM_NODOS; n++) {
                sem_wait(&sistema->nodos[n].mutex_consumo);

                if (sistema->nodos[n].consumo_total_dia > CONSUMO_CRITICO_LIMITE) {
                    
                    int es_industrial = (sistema->nodos[n].tipo_User == 1);
                    int justificado = 0;

                    if (es_industrial) {
                        justificado = (rand() % 100) < 85; // 85% justificado
                    } else {
                        justificado = (rand() % 100) < 55; // 55% justificado
                    }

                    if (justificado) {
                        sistema->auditor.consumos_criticos_validados++;
                        printf("   ✅ NODO %d: Consumo crítico VALIDADO\n", n + 1);
                    } else {
                        sistema->auditor.consumos_criticos_no_justificados++;
                        sistema->auditor.multas_generadas++;
                        printf("   ❌ NODO %d: Consumo crítico NO JUSTIFICADO - MULTA\n", n + 1);
                    }

                    int hora = sistema->nodos[n].hora_reserva;
                    if (hora >= HORA_INICIO && hora < HORA_FIN) {
                        consumos_por_hora[hora - HORA_INICIO]++;
                    }
                }

                sem_post(&sistema->nodos[n].mutex_consumo);
            }
        }

        // Calcular consumo por hora
        printf("\n📊 AUDITOR: Consumo por hora:\n");
        for (int h = 0; h < TOTAL_HORAS; h++) {
            printf("   %2d:00 - %d litros\n", HORA_INICIO + h, consumos_por_hora[h] * 100);
        }

        // Calcular eficiencia
        int nodos_ocupados = 0;
        int tiempo_espera_total = 0;
        int muestras = 0;

        for (int h = 0; h < TOTAL_HORAS; h++) {
            for (int n = 0; n < NUM_NODOS; n++) {
                if (sistema->bloques[h].usuarios_en_nodo[n] != -1) {
                    nodos_ocupados++;
                    tiempo_espera_total += 5 + nodos_ocupados;
                    muestras++;
                }
            }
        }

        float eficiencia = 0;
        if (muestras > 0) {
            eficiencia = (nodos_ocupados * 100.0f) / (TOTAL_HORAS * NUM_NODOS);
            sistema->auditor.eficiencia_global = eficiencia;
            sistema->auditor.tiempo_espera_promedio = tiempo_espera_total / muestras;
        }

        printf("\n📈 AUDITOR: Eficiencia = %.1f%% | Espera promedio = %d min\n\n",
               eficiencia, sistema->auditor.tiempo_espera_promedio);

        usleep(50000);
    }

    // Reporte final del auditor
    printf("\n📋 AUDITOR - REPORTE MENSUAL:\n");
    printf("   ✅ Consumos críticos validados: %d\n", sistema->auditor.consumos_criticos_validados);
    printf("   ❌ Consumos no justificados: %d\n", sistema->auditor.consumos_criticos_no_justificados);
    printf("   💰 Multas generadas: %d\n", sistema->auditor.multas_generadas);
    printf("   📊 Eficiencia promedio: %.1f%%\n", sistema->auditor.eficiencia_global);

    return NULL;
}

// ==================== PROCESO USUARIO (PRODUCTOR) ====================
void* proceso_usuario(void* arg) {
    UsuarioThreadData* data = (UsuarioThreadData*)arg;
    SistemaEcoFlow* sistema = data->sistema;

    printf("   👤 Usuario %d (%s) iniciado\n", 
           data->stats.id_usuario, 
           data->stats.tipo == 0 ? "Residencial" : "Industrial");

    while (sistema->simulacion_activa && sistema->dia_actual <= DIAS_SIMULACION) {
        
        // Verificar límite diario de solicitudes
        if (sistema->solicitudes_dia_actual >= MAX_SOLICITUDES_DIA) {
            usleep(100000);
            continue;
        }

        // Crear nueva solicitud
        Solicitud sol;
        sol.id_usuario = data->stats.id_usuario;
        sol.tipo_usuario = data->stats.tipo;
        sol.hora_solicitada = HORA_INICIO + (rand() % TOTAL_HORAS);
        sol.nodo_preferido = rand() % NUM_NODOS;
        sol.timestamp = time(NULL);
        sol.litros_consumir = 0; // Inicializar por seguridad

        // Decidir acción según probabilidades
        decidir_accion_usuario(&sol);

        // Si es consumo, definir litros según tipo
        if (sol.accion == ACCION_CONSUMO) {
            if (data->stats.tipo == 0) {
                sol.litros_consumir = 100 + (rand() % 300); // Residencial: 100-400L
            } else {
                sol.litros_consumir = 200 + (rand() % 800); // Industrial: 200-999L
            }
        }

        // Encolar solicitud (productor)
        sem_wait(&sistema->cola_solicitudes.lleno);
        sem_wait(&sistema->cola_solicitudes.mutex_cola);

        sistema->cola_solicitudes.solicitudes[sistema->cola_solicitudes.final] = sol;
        sistema->cola_solicitudes.final = (sistema->cola_solicitudes.final + 1) % MAX_SOLICITUDES_DIA;
        sistema->cola_solicitudes.count++;

        sem_post(&sistema->cola_solicitudes.mutex_cola);
        sem_post(&sistema->cola_solicitudes.vacio);

        // Actualizar contador de solicitudes del día
        sem_wait(&sistema->mutex_global);
        sistema->solicitudes_dia_actual++;
        sem_post(&sistema->mutex_global);

        // Actualizar estadísticas del usuario
        data->stats.solicitudes_realizadas++;

        // Pequeña pausa entre solicitudes
        usleep(rand() % 300000);
    }

    printf("   👋 Usuario %d finalizado\n", data->stats.id_usuario);
    return NULL;
}

#endif // PROCESO_HILO_H