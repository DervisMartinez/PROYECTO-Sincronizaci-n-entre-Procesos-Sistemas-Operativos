#ifndef PROCESO_HILO_H
#define PROCESO_HILO_H
#include<unistd.h>
#include"utilidades.h"
#include"structuras.h"
#include"user_process.h"



void procesar_solicitudes(void* arg){
    SistemaEcoFlow* sistema =(SistemaEcoFlow*)arg;

    printf("PROCESADOR DE SOLISITUD INICIADO\n");
    printf("consumiendo solicitud de la cola\n");

    while(sistema->simulacion_activa){
        
        //esperar a que este una solicitud en la cola por lo menos
        sem_wait(&sistema->cola_solicitudes.vacio);

        //acceso esclusivo bloqueado
        sem_wait(&sistema->cola_solicitudes.mutex_cola);

        Solicitud solicit = sistema->cola_solicitudes.solicitudes[sistema->cola_solicitudes.frente];

        //aumento de posicion frente de la cola
        sistema->cola_solicitudes.frente = (sistema->cola_solicitudes.frente +1)% MAX_SOLICITUDES_DIA;
        sistema->cola_solicitudes.count--;

        sem_post(&sistema->cola_solicitudes.mutex_cola);

        //espacio en la cola liberado
        sem_post(&sistema->cola_solicitudes.lleno);

        //mostrar la solicitud a procesar
        printf("Procesando: Usuario %d - %s para %d:00\n",solicit.id_usuario,accion_a_string(solicit.accion),solicit.hora_solicitada);


        //Ejecutar la accion del usuario
        switch (solicit.accion){

        case ACCION_RESERVA: reservar_nodo(sistema,solicit.id_usuario,solicit.hora_solicitada,solicit.tipo_usuario);//flata nodo preferido
             break;
        case ACCION_CONSUMO: consumir_agua(sistema,solicit.id_usuario,solicit.nodo_preferido,solicit.hora_solicitada,solicit.litros_consumir,solicit.tipo_usuario);
             break;
        case ACCION_CANCELACION: cancelar_reserva(sistema,solicit.hora_solicitada) ;
             break;
        case ACCION_CONSULTA: consultar_presion(sistema,solicit.hora_solicitada) ;
             break;
        case ACCION_PAGO: pagar_excedente(sistema,solicit.id_usuario,solicit.hora_solicitada,solicit.nodo_preferido);
             break;
        
        default: printf("Accion desconocida %s\n",accion_a_string(solicit.accion));
        break;
        } 

        usleep(50000); //0.05 segundos 
    }


    printf("Procesador de solicitudes finalizado\n");

}

void poceso_monitor(void*arg){

    SistemaEcoFlow* sistema = (SistemaEcoFlow*)arg;

    while (sistema->simulacion_activa) {
        
        sleep(3);

        //bloqueo de seccion critica
        sem_wait(&sistema->monitor.mutex_monitor);

        printf("MONITOR DE PRESION\n");

        for(int i=0; i<NUM_NODOS; i++){

            printf("NODO %d: Presion %3d psi %s",i+1,sistema->monitor.presion_actual[i],sistema->nodos[i].reservado?"OCUPADO":"LIBRE");
        }

        //estadisticas
            printf("Solicitudes: %3d/%-3d \n",sistema->solicitudes_dia_actual,MAX_SOLICITUDES_DIA);
            printf("Eficiencia: %.1f",sistema->auditor.eficiencia_global);

        //liberacion de recurso
        sem_post(&sistema->monitor.mutex_monitor);

    }
}

void control_dia (void*arg){

    SistemaEcoFlow* sistema = (SistemaEcoFlow*)arg;

    printf("CONTROLADOR DE DIA\n");

    //mistras no culminen los 30 dias
    while (sistema->dia_actual <=DIAS_SIMULACION){
        
        printf("DIA %d - INICIO (6:00 AM)\n",sistema->dia_actual);
        sleep(12); //12 segundos = 1 dia de simulacion (12 horas operando)
        printf("DIA %d -FIN (6:00 PM)\n",sistema->dia_actual);

        printf("Procesando ultimas solicitudes\n");
        while (sistema->cola_solicitudes.count>0) {
            sleep(1);
        }


        //limpiar cola para un nuevo dia
        limpiar_cola(&sistema->cola_solicitudes);

        //reiniciar contador solicitudes
        sistema->solicitudes_dia_actual =0;

        //liberacion de nodos en cada bloque,seccion critica
        liberar_nodos(sistema);

        //reiniciar consumo diario de los nodos, seccion critica
        liberar_consumo(sistema);

        sistema->dia_actual ++;

        if(sistema->dia_actual <= DIAS_SIMULACION){
            printf("Sistema listo para DIA %d\n\n",sistema->dia_actual);
        }

    }
    
    printf("SIMULACION COMPLETADA. %d dias transcurridos\n",DIAS_SIMULACION);
    sistema->simulacion_activa =0;
}

/*
//faltante 
void proceso_auditor(void*arg){


}

//faltante
void proceso_usuario(void*arg){


}
*/

#endif //PROCESO_HILO_H