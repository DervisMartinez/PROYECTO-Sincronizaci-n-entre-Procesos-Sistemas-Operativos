#ifndef PROCESOS_H
#define PROCESOS_H

#include "structuras.h"

// Hilos principales
void* hilo_auditor(void* arg);
void* hilo_monitor_presion(void* arg);
void* hilo_procesador_solicitudes(void* arg);
void* hilo_usuario(void* arg);
void* hilo_control_dias(void* arg);

// Funciones de usuario
int reservar_nodo(SistemaEcoFlow* sistema, int id_usuario, int hora, int tipo_usuario);
int cancelar_reserva(SistemaEcoFlow* sistema, int id_usuario);
int consultar_presion(SistemaEcoFlow* sistema, int id_usuario, int hora);
int consumir_agua(SistemaEcoFlow* sistema, int id_usuario, int litros, int justificado);
int pagar_tarifa(SistemaEcoFlow* sistema, int id_usuario);

// Funciones de sincronización (patrón lectores-escritores)
void iniciar_lectura(BloqueHorario* bloque);
void terminar_lectura(BloqueHorario* bloque);
void iniciar_escritura(BloqueHorario* bloque);
void terminar_escritura(BloqueHorario* bloque);

// Funciones de utilidad
int hora_actual();
void actualizar_eficiencia(SistemaEcoFlow* sistema);
void registrar_amonestacion(SistemaEcoFlow* sistema, int id_usuario);
int nodo_disponible(BloqueHorario* bloque, int hora, int nodo);

#endif