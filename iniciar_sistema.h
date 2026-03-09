#ifndef INICIALIZAR_SISTEMA_H
#define INICIALIZAR_SISTEMA_H
#include "structuras.h"

void inicializar_sistema_ecoflow(SistemaEcoFlow* sistema){
    inicializar_nodos(sistema);
    inicializar_bloques(sistema);
    inicializar_cola(&sistema->cola_solicitudes);
    inicializar_sistema(sistema);
}

#endif