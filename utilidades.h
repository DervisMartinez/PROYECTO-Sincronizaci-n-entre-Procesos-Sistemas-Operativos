#ifndef UTILIDADES_H
#define UTILIDADES_H
#include "structuras.h"

//// FALTA HACER REGISTRAR AMONESTACION
//// FALTA HACER REGISTRAR AMONESTACION
//// FALTA HACER REGISTRAR AMONESTACION
//// FALTA HACER REGISTRAR AMONESTACION
//// FALTA HACER REGISTRAR AMONESTACION

//procedimiento utilizado para limpiar la cola de solicitudes al final de cada dia.
// LIMPIAR_COLA
void limpiar_cola(ColaSolicitudes* cola);

//liberar consumo, para control del dia
void liberar_consumo(SistemaEcoFlow* sistema);

void liberar_nodos(SistemaEcoFlow* sistema);

// GENERAR_PROBABILIDAD
// Implementación de la Regla de Probabilidad (50% Reserva)
void decidir_accion_usuario(Solicitud* solicitud) ;

// LIMPIAR_SISTEMA
void limpiar_sistema(SistemaEcoFlow* sistema) ;

// GENERAR_REPORTE_MENSUAL
void generar_reporte_mensual(SistemaEcoFlow* sistema);

// VALIDAR_HORA
void validar_hora(int hora);

const char *accion_a_string(TipoAccion accion);
#endif