//Gabriel Silva
#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

// ============================================================================
// CONSTANTES GLOBALES

#define NUM_NODOS 10                // Válvulas principales
#define HORA_INICIO 6               // 6:00 AM
#define HORA_FIN 18                 // 6:00 PM
#define TOTAL_HORAS 12              // 6 a 17 (18 es cierre)
#define MAX_SOLICITUDES_DIA 250     // Máximo de solicitudes por día de alta demanda
#define DIAS_SIMULACION 30           // Un mes de simulación
#define CONSUMO_CRITICO_LIMITE 500   // Litros para considerar crítico
#define NUM_USUARIOS 20              // Total de usuarios (puede ajustarse)
#define NUM_RESIDENCIALES 12         // Cantidad de usuarios residenciales (ajustar segun la cantidad de usuarios en el sitema)
#define NUM_INDUSTRIALES 8           // Cantidad de usuarios industriales (ajustar segun la cantidad de usuarios,la suma junto con NUM_RESIDENCIALES = NUM_USUARIOS)

// ============================================================================
// TIPOS ENUMERADOS

typedef enum {
    ACCION_RESERVA,
    ACCION_CONSUMO,
    ACCION_CANCELACION,
    ACCION_CONSULTA,
    ACCION_PAGO
} TipoAccion;             //para asignar de forma aleatoria la accion que tomara el usuario para y asi formar su solicitud


// ============================================================================
// ESTRUCTURAS PRINCIPALES


// ----------------------------------------------------------------------------
// Nodo de Flujo (Recurso crítico)

typedef struct {
    int id_nodo;                  // identificador
    int reservado;               // 0=libre, 1=reservado  (no es estrictamente necesario, luego evaluamos si quitarlo)
    int usuario_actual;           // ID del usuario que lo reservó (-1 si libre)
    int hora_reserva;             // Hora para la que está reservado
    int consumo_total_dia;         // Litros consumidos en el día
    int consumo_total_mes;         // Litros consumidos en el mes
    int veces_reservado;           // Contador de reservas

    // Semáforos para exclusión mutua
    sem_t mutex_nodo;             // Protege operaciones sobre el nodo, en saso de ser reservado
    sem_t mutex_consumo;           // Protege actualizaciones de consumo, pra el proceso consumir agua, primero se verifica que no este reservado

    time_t ultima_modificacion;
} NodoFlujo;



// ----------------------------------------------------------------------------
// Bloque Horario (para disponibilidad por hora)

typedef struct {
    int hora;                      // 6,7,...,17
   // int nodos_disponibles[NUM_NODOS]; // 0=ocupado, 1=disponible (no es estrictamente necesario, evaluaremos si dejarlo o no, se puede manejar con nodos_disponibles)
    int usuarios_en_nodo[NUM_NODOS];   // ID del usuario que ocupa (-1 si libre)

    // Semáforos para patrón lectores-escritores
    sem_t mutex_lectores;           // Protege contador de lectores 
    sem_t escritor;                 // Semáforo para escritores, se bloquea para acciones como "consultarPresion"
    int lectores_activos;           // Número de lectores actuales

    // Estadísticas del bloque
    int reservas_realizadas;
    int consultas_realizadas;
} BloqueHorario;



// ----------------------------------------------------------------------------
// Solicitud de Usuario

typedef struct {
    int id_usuario;
    TipoAccion accion;
    int hora_solicitada;
    int nodo_preferido;             // -1 si no hay preferencia
    int tipo_usuario;                // 0=residencial, 1=industrial
    int litros_consumir;             // Para acción CONSUMO
    time_t timestamp;
} Solicitud;


// ----------------------------------------------------------------------------
// Cola de Solicitudes (Productor‑Consumidor)

typedef struct {
    Solicitud solicitudes[MAX_SOLICITUDES_DIA];
    int frente;
    int final;
    int count;

    sem_t mutex_cola;                // Exclusión mutua para la cola
    sem_t lleno;                     // Indica espacios disponibles ( lleno y vacio son usados pra productor="usuarios", consumidor="procesadorSolicitudes")
    sem_t vacio;                      // Indica elementos disponibles
} ColaSolicitudes;


// ----------------------------------------------------------------------------
// Estadísticas del Auditor

typedef struct {
    int consumos_criticos_validados; //(por emergencia o industria creitica)
    int consumos_criticos_no_justificados;
    int multas_generadas;
    float eficiencia_global;
    int nodos_ocupados_promedio;
    int tiempo_espera_promedio;

    sem_t mutex_auditoria; //usado para recibir la notificacion de consumo irregular
} AuditorFlujo;


// ----------------------------------------------------------------------------
// Datos del Monitor de Presión

typedef struct {
    int presion_actual[NUM_NODOS];    // Simulación de presión
    int caudal_actual[NUM_NODOS];      // Simulación de caudal, (cantidad de agua flujendo dependiendo del estado del no, ocupado o libre)
    int alertas_activas;
    char dashboard[2048]; // guardar todo antes de mostrar cada cierto tiempo, (opcional)
    sem_t mutex_monitor;  
} MonitorPresion;


// ----------------------------------------------------------------------------
// Estadísticas de un Usuario (para seguimiento)

typedef struct {
    int id_usuario;
    int tipo;                         // 0=residencial, 1=industrial
    int solicitudes_realizadas;
    int reservas_exitosas;
    int cancelaciones;
    int amonestaciones_recibidas;
    int consumo_total;
} UsuarioStats;


// ----------------------------------------------------------------------------
// Datos para hilos de usuario (contiene puntero al sistema)

typedef struct {
    UsuarioStats stats;
    struct SistemaEcoFlow* sistema;
} UsuarioThreadData;



// ----------------------------------------------------------------------------
// Sistema Global (contiene todos los componentes)

typedef struct SistemaEcoFlow {
    NodoFlujo nodos[NUM_NODOS];
    BloqueHorario bloques[TOTAL_HORAS];
    ColaSolicitudes cola_solicitudes;
    int solicitudes_dia_actual;
    int dia_actual;
    int simulacion_activa;

    AuditorFlujo auditor;
    MonitorPresion monitor;

    // Estadísticas globales
    int total_metros_cubicos;
    int total_amonestaciones;
    int total_consumos_criticos;
    int total_consumos_estandar;
    int total_reservas_exitosas;
    int total_reservas_fallidas;

    // Semáforos globales
    sem_t mutex_global;
    sem_t mutex_estadisticas;
    sem_t mutex_dia;

    time_t tiempo_inicio;
} SistemaEcoFlow;


// ============================================================================
// PROTOTIPOS DE FUNCIONES DE INICIALIZACIÓN

void inicializar_nodos(SistemaEcoFlow* sistema);
void inicializar_bloques(SistemaEcoFlow* sistema);
void inicializar_cola(ColaSolicitudes* cola);
void inicializar_sistema(SistemaEcoFlow* sistema);
void limpiar_cola(ColaSolicitudes* cola);

//falta ser implementada
void generar_reporte_mensual(SistemaEcoFlow* sistema);



#endif // ESTRUCTURAS_H