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
    
    pthread_t hilo_auditor, hilo_monitor, hilo_dia, hilo_procesador;
    pthread_t hilos_usuarios[NUM_USUARIOS];
    UsuarioThreadData usuarios_data[NUM_USUARIOS];
    
    pthread_create(&hilo_auditor, NULL, proceso_auditor, &sistema);
    pthread_create(&hilo_monitor, NULL, proceso_monitor, &sistema);
    pthread_create(&hilo_dia, NULL, control_dia, &sistema);
    pthread_create(&hilo_procesador, NULL, procesar_solicitudes, &sistema);
    
    for(int i = 0; i < NUM_USUARIOS; i++) {
        usuarios_data[i].stats.id_usuario = 1000 + i;
        usuarios_data[i].stats.tipo = (i < NUM_RESIDENCIALES) ? 0 : 1;
        usuarios_data[i].sistema = &sistema;
        pthread_create(&hilos_usuarios[i], NULL, proceso_usuario, &usuarios_data[i]);
    }
    
    pthread_join(hilo_dia, NULL);
    
    sistema.simulacion_activa = 0;
    
    pthread_join(hilo_procesador, NULL);
    for(int i = 0; i < NUM_USUARIOS; i++) {
        pthread_join(hilos_usuarios[i], NULL);
    }
    
    pthread_cancel(hilo_auditor);
    pthread_cancel(hilo_monitor);
    
    return 0;
}