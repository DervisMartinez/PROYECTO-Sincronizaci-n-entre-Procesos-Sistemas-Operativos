#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "structuras.h"
#include "iniciar_sistema.h"
#include "procesos_hilos.h"
#include "user_process.h"


int main() {
    srand(time(NULL));
    
    SistemaEcoFlow sistema;
    inicializar_sistema_ecoflow(&sistema);
    
    pthread_t hilo_auditor, hilo_monitor, hilo_dia;
    pthread_t hilo_procesador[NUM_PROCESADORES];
    pthread_t hilos_usuarios[NUM_USUARIOS];
    UsuarioThreadData usuarios_data[NUM_USUARIOS];
    
    // Crear hilos
    pthread_create(&hilo_auditor, NULL, proceso_auditor, &sistema);
    pthread_create(&hilo_monitor, NULL, proceso_monitor, &sistema);
    pthread_create(&hilo_dia, NULL, control_dia, &sistema);
    
    for(int h = 0; h < NUM_PROCESADORES; h++) {
        pthread_create(&hilo_procesador[h], NULL, procesar_solicitudes, &sistema); 
    }

    for(int i = 0; i < NUM_USUARIOS; i++) {
        usuarios_data[i].stats.id_usuario = 1000 + i;
        usuarios_data[i].stats.tipo = (i < NUM_RESIDENCIALES) ? 0 : 1;
        usuarios_data[i].sistema = &sistema;
        pthread_create(&hilos_usuarios[i], NULL, proceso_usuario, &usuarios_data[i]);
    }
    
    // Esperar fin de simulación
    pthread_join(hilo_dia, NULL);
    
    //SEÑALAR FIN 
    sistema.simulacion_activa = 0;
    
    // DESPERTAR PROCESADORES BLOQUEADOS 
    printf(" Despertando procesadores...\n");
    for(int h = 0; h < NUM_PROCESADORES; h++) {
        sem_post(&sistema.cola_solicitudes.vacio); 
    }
    
    // ESPERAR PROCESADORES 
    for(int h = 0; h < NUM_PROCESADORES; h++) {
        pthread_join(hilo_procesador[h], NULL);
    }

    //ESPERAR USUARIOS 
    for(int i = 0; i < NUM_USUARIOS; i++) {
        pthread_join(hilos_usuarios[i], NULL);
    }
    
    // CANCELAR Y ESPERAR AUDITOR Y MONITOR
    pthread_cancel(hilo_auditor);
    pthread_cancel(hilo_monitor);
    
    pthread_join(hilo_auditor, NULL);
    pthread_join(hilo_monitor, NULL);

    generar_reporte_mensual(&sistema);
    limpiar_sistema(&sistema);

    printf("\nFIN - Simulación terminada correctamente\n\n");
    
    return 0;
}

    