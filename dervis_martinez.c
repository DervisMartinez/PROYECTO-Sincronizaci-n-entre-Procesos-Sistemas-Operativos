#include <stdio.h>
#include "structuras.h"


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

