#include<unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "structuras.h"
#include "utilidades.h"


// ==================== FUNCIONES DE USUARIO ====================


// RESERVAR NODO 
void reservar_nodo(SistemaEcoFlow* sistema, int id_usuario, int hora , int tipo_usuario, int nodo_preferido) {

    if (hora < HORA_INICIO || hora >= HORA_FIN) return ; 
    
    int idx_hora = hora - HORA_INICIO;
    BloqueHorario* bloque = &sistema->bloques[idx_hora];
    
    // SECCIÓN CRÍTICA: Acceso exclusivo para modificar disponibilidad
    sem_wait(&bloque->escritor); 
    
    int nodo_asignado = -1;

    if(nodo_preferido != -1){

        if(nodo_preferido >=0 && nodo_preferido <NUM_NODOS){

            if(bloque->usuarios_en_nodo[nodo_preferido]== -1){ nodo_asignado = nodo_preferido;}
        }

    }else{

        for (int i = 0; i < NUM_NODOS; i++) {
            if (bloque->usuarios_en_nodo[i] == -1) {
                nodo_asignado = i;
                break;
            }
        }

    }

    if (nodo_asignado != -1) {
        // Actualizar estado en el bloque horario
        bloque->usuarios_en_nodo[nodo_asignado] = id_usuario;
        bloque->reservas_realizadas++;
        
        // Sincronizar actualización en la estructura del nodo específico
        sem_wait(&sistema->nodos[nodo_asignado].mutex_nodo);
        sistema->nodos[nodo_asignado].reservado = 1;
        sistema->nodos[nodo_asignado].tipo_User = tipo_usuario;
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
    
}

// CONSUMIR_AGUA
void consumir_agua (SistemaEcoFlow* sistema, int usuario_id, int nodo_id, int hora, int litros, int tipo_usuario){

    validar_hora(hora);

    int hora_actual = hora - HORA_INICIO;
    int consumo_valido = 0;

    //verificar reserva del usuario en el nodo en la hora proporcionada

    //bloquear el recurso como lector
    sem_wait(&sistema->bloques[hora_actual].mutex_lectores);
    sistema->bloques[hora_actual].lectores_activos ++;

    if(sistema->bloques[hora_actual].lectores_activos == 1){
        //bloquear posibles escritores en este bloque
        sem_wait(&sistema->bloques[hora_actual].escritor);
    }

    sem_post(&sistema->bloques[hora_actual].mutex_lectores);

    //verificacion de reservaa
    if(sistema->bloques[hora_actual].usuarios_en_nodo[nodo_id] == usuario_id){

        consumo_valido = 1;
    }

    //liberacion de lectores
    sem_wait(&sistema->bloques[hora_actual].mutex_lectores);
    sistema->bloques[hora_actual].lectores_activos --;

    if(sistema->bloques[hora_actual].lectores_activos == 0){

        //dar paso a los escritores
        sem_post(&sistema->bloques[hora_actual].escritor);
    }
    sem_post(&sistema->bloques[hora_actual].mutex_lectores);


    if(consumo_valido){

        //bloquear er recurso consumo dentro del nodo, exclusion mutua
        sem_wait(&sistema->nodos[nodo_id].mutex_consumo);

        //actualizacion de consumo
        sistema->nodos[nodo_id].consumo_total_dia += litros;
        sistema->nodos[nodo_id].consumo_total_mes += litros;

        //actualizacion de estadisticas mensual
        sem_wait(&sistema->mutex_estadisticas);
        sistema->total_metros_cubicos += litros / 1000;


        //clasificacion de consumo, (critico > 500 litros)
        if(litros > CONSUMO_CRITICO_LIMITE){
            
            sistema->total_consumos_criticos ++;

            //notificando al auditor por exceso
            sem_post(&sistema->auditor.mutex_auditoria);

            printf("CONSUMO CRITICO %d litros - Auditor notificado\n",litros);

        }else{

            sistema->total_consumos_estandar ++;
        }

        sem_post(&sistema->mutex_estadisticas);

        //mostrar consumo realizado
        printf("%s %d consumio %d litros en NODO %d (%d:00)\n",tipo_usuario== 0? "Residencial":"Industrial",usuario_id,litros,nodo_id,hora);

        //liberar mutex de consumo
        sem_post(&sistema->nodos[nodo_id].mutex_consumo);
    }else{

        printf("EL usuario %d No tiene reserca en el NODO %d para %d:00\n", usuario_id, nodo_id, hora);
        sem_post(&sistema->nodos[nodo_id].mutex_consumo);

    }

}

// CANCELAR_RESERVA
void cancelar_reserva(SistemaEcoFlow* sistema, int id_usuario) {

    int cancelado =0;
    //validar_hora(hora); //en saso de que se agg una bloque horario
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
                sistema->nodos[i].tipo_User = -1;
                sistema->nodos[i].usuario_actual = -1;
                sistema->nodos[i].hora_reserva = -1;
                sistema->nodos[i].ultima_modificacion = time(NULL);
                sem_post(&sistema->nodos[i].mutex_nodo);
                
                printf("[SISTEMA] Usuario %d -> Cancelación exitosa Nodo %d\n", id_usuario, i);
                cancelado =1;
                break;; 
            }
        }
        sem_post(&bloque->escritor);
    }
    
    if(!cancelado){

        //actualizar estadisticas de amonestaciones
        sem_wait(&sistema->mutex_estadisticas);
        sistema->total_amonestaciones ++;
        sem_post(&sistema->mutex_estadisticas);
        printf("[ALERTA] Usuario %d -> Amonestado por cancelación inválida\n", id_usuario);
        
    }

    
}

// CONSULTAR_PRESION // falta revision como trabajaremos la presion 
void consultar_presion(SistemaEcoFlow* sistema, int hora) {
    BloqueHorario* bloque = &sistema->bloques[hora - HORA_INICIO];

    // Entrada del Lector
    sem_wait(&bloque->mutex_lectores);
    bloque->lectores_activos++;
    if (bloque->lectores_activos == 1) {
        sem_wait(&bloque->escritor); // El primer lector bloquea a los escritores
    }
    sem_post(&bloque->mutex_lectores);

    //MEJORAR
    //---------------------------------------------------------------------------------------------------------
    // SECCIÓN CRÍTICA DE LECTURA (Simultánea)
    printf("[LECTOR] Usuario consultando presión en bloque %d:00... OK\n", hora);
    usleep(100000); // Simula el tiempo de la comunicación inalámbrica

    //---------------------------------------------------------------------------------------------------------

    
    // Salida del Lector
    sem_wait(&bloque->mutex_lectores);
    bloque->lectores_activos--;
    if (bloque->lectores_activos == 0) {
        sem_post(&bloque->escritor); // El último lector libera a los escritores
    }
    sem_post(&bloque->mutex_lectores);
}

// PAGAR_TAFIRA_EXCEDENTE
void pagar_excedente (SistemaEcoFlow* sistema,int usuario_id, int hora, int nodo_id){

    validar_hora(hora);

    int hora_actual = hora - HORA_INICIO;

    //bloquear como escritor, en caso de que el nodo si este reservado por el usuario se paga para liberar lo que implica modificacion
    sem_wait(&sistema->bloques[hora_actual].escritor);

    //bloquear el nodo en espesifico, para verificar que este reservado por el usuario y modificar si asi lo es 
    sem_wait(&sistema->nodos[nodo_id].mutex_nodo);
    
    //revisar bien este condicional, posible verificacion doble
    if(sistema->nodos[nodo_id].usuario_actual == usuario_id && sistema->bloques[hora_actual].usuarios_en_nodo[nodo_id] ==usuario_id){

        //registrar el pago
        printf("Usuario %d pago el excedente por NODO %d (%d:00)\n",usuario_id,nodo_id,hora);

        //liberacion del nodo
        sistema->bloques[hora_actual].usuarios_en_nodo[nodo_id] = -1;
        
        sistema->nodos[nodo_id].reservado = 0;
        sistema->nodos[nodo_id].usuario_actual = -1;
        sistema->nodos[nodo_id].tipo_User = -1;
        sistema->nodos[nodo_id].hora_reserva = -1;
        sistema->nodos[nodo_id].ultima_modificacion= time(NULL);

    }else{

        printf("El usuario %d no tiene reserva en NODO %d para %d:00\n",usuario_id,nodo_id,hora);
    }

    sem_post(&sistema->bloques[hora_actual].escritor);
    sem_post(&sistema->nodos[nodo_id].mutex_nodo);
}


