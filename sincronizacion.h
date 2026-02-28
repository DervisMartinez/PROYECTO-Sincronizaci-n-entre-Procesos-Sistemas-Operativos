//Dervis Martinez
#ifndef SINCRONIZACION_H
#define SINCRONIZACION_H

#include <semaphore.h>

extern sem_t mutex_nodo;       // Exclusión mutua para reservar/cancelar
extern sem_t mutex_lectores;    // Mutex para contador de lectores
extern sem_t base_datos;        // Semáforo para lectores/escritores
extern int lectores_activos;

void inicializar_semaforos();
void destruir_semaforos();

#endif // SINCRONIZACION_H