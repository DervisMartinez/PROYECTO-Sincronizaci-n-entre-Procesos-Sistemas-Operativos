#include<stdio.h>
#include"structuras.h"


void inicializar_nodos(SistemaEcoFlow* sistema){

    for(int i=0; i<NUM_NODOS; i++){

        sistema->nodos[i].id_nodo = i;
        sistema->nodos[i].reservado = 0;
        sistema->nodos[i].usuario_actual = -1;
        sistema->nodos[i].hora_reserva = 0;
        sistema->nodos[i].consumo_total_dia = 0;
        sistema->nodos[i].consumo_total_mes = 0;
        sistema->nodos[i].ultima_modificacion = time(NULL);

        sem_init(&sistema->nodos[i].mutex_nodo,0,1);  // activo al comienzo
        sem_init(&sistema->nodos[i].mutex_consumo,0,1); //activo al comienzo

    }
}



void inicializar_bloques(SistemaEcoFlow* sistema){

   for(int i=0; i<TOTAL_HORAS; i++){

        sistema->bloques[i].hora = HORA_INICIO + i;
        // NOTAA elimine el arreglo nodos_disponibles dentro de bloque horario, nos basta con Usuarios_en_nodo
        //marcar todos los nodos disponibles dentro de cada bloque horario
        for(int h=0; h<NUM_NODOS; h++){

            sistema->bloques[i].usuarios_en_nodo[h] = -1;

        }

        sistema->bloques[i].lectores_activos = 0;
        sistema->bloques[i].reservas_realizadas = 0;
        sistema->bloques[i].consultas_realizadas = 0;

        sem_init(&sistema->bloques[i].mutex_lectores,0,1); //activo al inicio
        sem_init(&sistema->bloques[i].escritor,0,1); //activo al inicio
    } 

}


void inicializar_cola(ColaSolicitudes* cola){

    cola->frente = 0;
    cola-> final = 0;
    cola-> count = 0;

    //NOTA. no inicialice el arreglo solicitudes, solo se reserva como un espacio de memoria, se inicia cada pocicion en proceso hilo usuario

    sem_init(&cola->mutex_cola,0,1);

    //semaforos para manejo de productor consumidor, con bufer acotado
    sem_init(&cola->lleno,0,MAX_SOLICITUDES_DIA); //todos los espacios disponible
    sem_init(&cola->vacio,0,0); //ningun elemento
}


void inicializar_sistema(SistemaEcoFlow* sistema){

    //inicializar auditor
    sistema->auditor.consumos_criticos_no_justificados = 0;
    sistema->auditor.consumos_criticos_validados = 0;
    sistema->auditor.multas_generadas = 0;
    sistema->auditor.eficiencia_global = 0.0;
    sistema->auditor.nodos_ocupados_promedio = 0;
    sistema->auditor.tiempo_espera_promedio = 0;
    sem_init(&sistema->auditor.mutex_auditoria,0,0);

    //inicializar monitor
    for(int i=0; i<NUM_NODOS; i++){

        sistema->monitor.presion_actual[i] = 50 + rand() % 50;
        sistema->monitor.caudal_actual[i] = 0;
        
    }

    sistema->monitor.alertas_activas = 0;
    //memset(); // terminar limpiado de dashboard, no recuerdo jajaja
    //xddd
    sem_init(&sistema->monitor.mutex_monitor,0,1);


    //inicializar el resto de variables de sistema


}


//procedimiento utilizado para limpiar la cola de solicitudes al final de cada dia.
void limpiar_cola(ColaSolicitudes* cola){

    cola->frente = 0;
    cola->final = 0; 
    cola->count = 0;

    //volver a activar el semaforo de exclusion mutua
    sem_post(&cola->mutex_cola);

    //destruir semaforos de productoor consumidor
    sem_destroy(&cola->lleno);
    sem_destroy(&cola->vacio);

    //crear de nuevo los semaforos, creo que es mejor asi que intentar limpiarllos
    sem_init(&cola->lleno,0,MAX_SOLICITUDES_DIA);
    sem_init(&cola->vacio,0,0);
}




//ULTIMAS AGREGACIONES -------------------------------------------------------------------------------------------

// validador de hora
void validar_hora(int hora){

     if(hora < HORA_INICIO || hora > HORA_FIN){

        printf("Hora %d:00 fuera de horario\n",hora);
        return;

    }
}

//PROCESO USUARIO. PAGAR PARA LA LIBERACION DEL NODO

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

    }else{

        printf("El usuario %d no tiene reserva en NODO %d para %d:00\n",usuario_id,nodo_id,hora);
    }

    sem_post(&sistema->bloques[hora_actual].escritor);
    sem_post(&sistema->nodos[nodo_id].mutex_nodo);
}


//PROCESO USUARIO. consumir 

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


    //RESERVA valida, generar consumo.  INCOMPLETO
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

        printf("EL usuario %d No tiene reserca en el NODO %d para %d:00\n");
        sem_post(&sistema->nodos[nodo_id].mutex_consumo);

    }

}









