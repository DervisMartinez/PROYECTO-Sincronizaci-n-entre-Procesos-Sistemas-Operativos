#ifndef PROCESO_USUARIO_H
#define PROCESO_USUARIO_H
#include "structuras.h"
#include "utilidades.h"

void reservar_nodo(SistemaEcoFlow* sistema, int id_usuario, int hora, int tipo_usuario,int nodo_preferido);
void consumir_agua (SistemaEcoFlow* sistema, int usuario_id, int nodo_id, int hora, int litros, int tipo_usuario);
void cancelar_reserva(SistemaEcoFlow* sistema, int id_usuario);
void consultar_presion(SistemaEcoFlow* sistema, int hora);
void pagar_excedente (SistemaEcoFlow* sistema,int usuario_id, int hora, int nodo_id);

#endif //PROCESO_USUARIO_H