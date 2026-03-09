#ifndef PROCESO_HILO_H
#define PROCESO_HILO_H
#include<unistd.h>
#include"utilidades.h"
#include"structuras.h"
#include"user_process.h"



void* procesar_solicitudes(void* arg){
    SistemaEcoFlow* sistema =(SistemaEcoFlow*)arg;

    printf("\nPROCESADOR DE SOLISITUD INICIADO\n");
    printf("\nconsumiendo solicitud de la cola\n\n");

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

        case ACCION_RESERVA: reservar_nodo(sistema,solicit.id_usuario,solicit.hora_solicitada,solicit.tipo_usuario,solicit.nodo_preferido);//flata nodo preferido
             break;
        case ACCION_CONSUMO: consumir_agua(sistema,solicit.id_usuario,solicit.nodo_preferido,solicit.hora_solicitada,solicit.litros_consumir,solicit.tipo_usuario);
             break;
        case ACCION_CANCELACION: cancelar_reserva(sistema,solicit.id_usuario) ;
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


    printf("\nProcesador de solicitudes finalizado\n");

     return NULL;
}

void* proceso_monitor(void*arg){

    SistemaEcoFlow* sistema = (SistemaEcoFlow*)arg;

    while (sistema->simulacion_activa) {
        
        sleep(10);

        //bloqueo de seccion critica
        sem_wait(&sistema->monitor.mutex_monitor);

        printf("\nMONITOR DE PRESION\n");

        for(int i=0; i<NUM_NODOS; i++){

            printf("NODO %d: Presion %3d psi %s\n",i+1,sistema->monitor.presion_actual[i],sistema->nodos[i].reservado?"OCUPADO":"LIBRE");
        }

        //estadisticas
            printf("Solicitudes: %3d/%-3d \n",sistema->solicitudes_dia_actual,MAX_SOLICITUDES_DIA);
            printf("Eficiencia: %.1f\n\n",sistema->auditor.eficiencia_global);

        //liberacion de recurso
        sem_post(&sistema->monitor.mutex_monitor);

    }
     return NULL;
}

void* control_dia (void*arg){

    SistemaEcoFlow* sistema = (SistemaEcoFlow*)arg;

    printf("\n");
    printf("CONTROLADOR DE DIA\n");
    printf("   Simulando %d días\n\n", DIAS_SIMULACION);

    //mistras no culminen los 30 dias
    while (sistema->dia_actual <=DIAS_SIMULACION){
        
        printf("DIA %d - INICIO (6:00 AM)\n\n",sistema->dia_actual);
        sleep(12); //12 segundos = 1 dia de simulacion (12 horas operando)
        printf("DIA %d -FIN (6:00 PM)\n\n",sistema->dia_actual);

        printf("Procesando ultimas solicitudes\n\n");
        while (sistema->cola_solicitudes.count>0) {
            sleep(1);
        }


        //limpiar cola para un nuevo dia
        printf(" Limpiando cola de solicitudes...\n");
        limpiar_cola(&sistema->cola_solicitudes);
        

        //reiniciar contador solicitudes
        sistema->solicitudes_dia_actual =0;

        //liberacion de nodos en cada bloque,seccion critica
        printf("Liberando nodos...\n");
        liberar_nodos(sistema);

        //reiniciar consumo diario de los nodos, seccion critica
        printf("Reiniciando consumo diario...\n");
        liberar_consumo(sistema);

        sistema->dia_actual ++;

        if(sistema->dia_actual <= DIAS_SIMULACION){
            printf("Sistema listo para DIA %d\n\n",sistema->dia_actual);
        }

    }
    
    printf("SIMULACION COMPLETADA. %d dias transcurridos\n",DIAS_SIMULACION);
    sistema->simulacion_activa =0;

     return NULL;
}

void* proceso_auditor(void* arg) {

    SistemaEcoFlow* sistema = (SistemaEcoFlow*)arg;
    
    printf("\n\nAUDITOR DE FLUJO iniciado\n");
    printf("\nSupervisando consumos críticos (>%dL)\n", CONSUMO_CRITICO_LIMITE);
    
    while(sistema->simulacion_activa) {
        
        // ESPERAR NOTIFICACIÓN DE CONSUMO CRÍTICO 
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 15;  // Revisar cada 15 segundos
        
        //esprea a que llegue la senal o se agoten los 15 segundos, si llega senal retorna 0, sino retorna -1
        int notificado = sem_timedwait(&sistema->auditor.mutex_auditoria, &ts);
        
        if(notificado == 0) {
            printf("\nAUDITOR: Validando consumos críticos pendientes...\n");
            
            for(int n = 0; n < NUM_NODOS; n++) {
                //bloquear recurso compartido,"consumo de cada nodo"
                sem_wait(&sistema->nodos[n].mutex_consumo);
                
                if(sistema->nodos[n].consumo_total_dia > CONSUMO_CRITICO_LIMITE) {
                    
                    // Determinar si está justificado (industrial = más probable)
                    int es_industrial = (sistema->nodos[n].tipo_User == 1);
                   // int id_usuario = sistema->nodos[n].usuario_actual;
                   

                    //determinar la probabilidad de justificacion
                    int justificado = 0;
                    
                    if(es_industrial) {
                        // Industriales tienen 85% de probabilidad de justificación
                        justificado = (rand() % 100) < 85;
                    } else {
                        // Residenciales solo 55% (emergencias reales)
                        justificado = (rand() % 100) < 55;
                    }
                    
                    if(justificado) {
                        sistema->auditor.consumos_criticos_validados++;
                        printf("NODO %d: Consumo crítico VALIDADO (justificado)\n", n+1);
                    } else {
                        sistema->auditor.consumos_criticos_no_justificados++;
                        sistema->auditor.multas_generadas++;
                        printf("NODO %d: Consumo crítico NO JUSTIFICADO - MULTA\n", n+1);
                    }
                    
                }
                
                sem_post(&sistema->nodos[n].mutex_consumo);
            }
        }
        


        // CALCULAR CONSUMO POR HORA

        static int contador = 0;

        if (contador %2 == 0 ){

            printf("\nAUDITOR CONSUMO POR HORA\n");
            printf("    HORA    |   LItro   |   CRITICOS    \n");
            printf("----------------------------------------\n");

            int total_litro=0;
            int total_critico=0;

            for(int i=0; i<TOTAL_HORAS; i++){

                printf("    %2d:00  |%6d litros  |   %2d\n", HORA_INICIO + i, sistema->auditor.consumo_por_hora[i], sistema->auditor.consumo_critico_por_hora[i] );

                total_litro += sistema->auditor.consumo_por_hora[i];
                total_critico+= sistema->auditor.consumo_critico_por_hora[i];
            }

            printf("    TOTAL  | %dL  |   %d\n\n",total_litro,total_critico);

        }
       
            
        
        //CALCULAR EFICIENCIA (nodos ocupados vs tiempo espera) 
        int nodos_ocupados = 0;
        int tiempo_espera_total = 0;
        int muestras = 0;
        
        for(int h = 0; h < TOTAL_HORAS; h++) {
            for(int n = 0; n < NUM_NODOS; n++) {

                if(sistema->bloques[h].usuarios_en_nodo[n] != -1) {
                    nodos_ocupados++;
                    // Simular tiempo de espera (mayor si muchos nodos ocupados)
                    tiempo_espera_total += 5 + (nodos_ocupados );
                    muestras++;
                }
            }
        }

        
        float eficiencia = 0;
        if(muestras > 0) {
            eficiencia = (nodos_ocupados * 100.0f) / (TOTAL_HORAS * NUM_NODOS);
            sistema->auditor.eficiencia_global = eficiencia;
            sistema->auditor.tiempo_espera_promedio = tiempo_espera_total / muestras;
            sistema->auditor.nodos_ocupados_promedio = (nodos_ocupados * 100) / (TOTAL_HORAS * NUM_NODOS);
        }
        
        printf("AUDITOR: Eficiencia = %.1f%% | Espera promedio = %d min \n\n", 
               eficiencia, sistema->auditor.tiempo_espera_promedio);
        
        usleep(50000);
    }
    
    return NULL;
}

void* proceso_usuario(void* arg) {
    UsuarioThreadData* data = (UsuarioThreadData*)arg;
    SistemaEcoFlow* sistema = data->sistema;
    
    while(sistema->simulacion_activa && sistema->dia_actual <= DIAS_SIMULACION) {
        
        // Verificar límite diario de solicitudes
        if(sistema->solicitudes_dia_actual >= MAX_SOLICITUDES_DIA) {
            usleep(100000);
            continue;
        }
      
        // Crear nueva solicitud
        Solicitud sol;
        sol.id_usuario = data->stats.id_usuario;
        sol.tipo_usuario = data->stats.tipo;
        sol.hora_solicitada = HORA_INICIO + (rand() % TOTAL_HORAS);
        
        // 70% de probabilidad de tener un nodo favorito, 30% de que le dé igual (-1)

        if ((rand() % 100) < 70) {
            sol.nodo_preferido = rand() % NUM_NODOS; 
        } else {
            sol.nodo_preferido = -1; // Sin preferencia, buscará cualquier disponible
        }
        // --------------------------

        sol.timestamp = time(NULL);
        
        decidir_accion_usuario(&sol);
        
        if(sol.accion == ACCION_CONSUMO) {
            if(data->stats.tipo == 0) {
                sol.litros_consumir = 100 + (rand() % 300);
            } else {
                sol.litros_consumir = 200 + (rand() % 800);
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
    
    return NULL;
}

#endif //PROCESO_HILO_H