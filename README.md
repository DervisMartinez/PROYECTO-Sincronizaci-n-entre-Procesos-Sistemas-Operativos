# 🌊 **ECO-FLOW 2026: Sistema de Gestión Hídrica Inteligente**

[![Sistemas Operativos](https://img.shields.io/badge/Asignatura-Sistemas%20Operativos-blue)](https://github.com)
[![Lenguaje](https://img.shields.io/badge/Lenguaje-C-green)](https://en.cppreference.com)
[![Sincronización](https://img.shields.io/badge/Sincronización-Semáforos-orange)](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/semaphore.h.html)

---

## 📋 **Tabla de Contenidos**
- [Descripción del Proyecto](#-descripción-del-proyecto)
- [Contexto y Ley](#-contexto-y-ley)
- [Arquitectura del Sistema](#-arquitectura-del-sistema)
- [Componentes Principales](#-componentes-principales)
- [Mecanismos de Sincronización](#-mecanismos-de-sincronización)
- [Resultados de la Simulación](#-resultados-de-la-simulación)
- [Compilación y Ejecución](#-compilación-y-ejecución)
- [Estructura del Proyecto](#-estructura-del-proyecto)
- [Autores](#-autores)

---

## 🎯 **Descripción del Proyecto**

**Eco-Flow 2026** es una simulación concurrente de un sistema inteligente de gestión de recursos hídricos, desarrollado como proyecto para la asignatura **Sistemas Operativos (CAO503)** de la Universidad de Carabobo.

El sistema modela el funcionamiento de un centro de control hídrico con **10 nodos de flujo** (válvulas principales) que deben ser gestionados por múltiples usuarios (residenciales e industriales) de manera concurrente, respetando reglas de sincronización y políticas de uso establecidas por la *Ley de Preservación de Recursos Hídricos de 2026*.

---

## 📜 **Contexto y Ley**

En el marco de la crisis climática global, el estado ha promulgado la **Ley de Preservación de Recursos Hídricos de 2026**, que establece:

> *"El suministro de agua en zonas residenciales e industriales debe ser gestionado por un sistema inteligente de 'nodos de flujo' para evitar el desperdicio mediante la asignación dinámica de presión."*

El sistema opera mediante dispositivos electrónicos incrustados en las tuberías matrices que se comunican de forma inalámbrica para autorizar el paso del líquido.

---

## 🏗️ **Arquitectura del Sistema**

---

## 🧩 **Componentes Principales**

### 1. **Usuario** (`proceso_usuario`)
- **Cantidad**: 20 hilos (12 residenciales, 8 industriales)
- **Función**: Generar solicitudes y encolarlas
- **Comportamiento**:
  - 50% probabilidad de reserva (según ley)
  - 70% probabilidad de tener nodo preferido
  - Residencial: 100-400L por consumo
  - Industrial: 200-999L por consumo

### 2. **Procesador de Solicitudes** (`procesar_solicitudes`)
- **Cantidad**: 4 hilos (consumidores)
- **Función**: Extraer solicitudes de la cola y ejecutar acciones
- **Acciones**: Reserva, Consumo, Cancelación, Consulta, Pago

### 3. **Auditor de Flujo** (`proceso_auditor`)
- **Función**: Supervisar consumos críticos (>500L)
- **Validación**:
  - Industrial: 85% justificado
  - Residencial: 55% justificado
- **Estadísticas**: Consumos validados, no justificados, multas

### 4. **Monitor de Presión** (`proceso_monitor`)
- **Función**: Mostrar estado del sistema cada 10 segundos
- **Información**: Presión por nodo, estado (libre/ocupado), eficiencia

### 5. **Controlador de Día** (`control_dia`)
- **Función**: Avanzar los días de simulación
- **Duración**: 30 días, cada día = 12 segundos
- **Tareas**: Limpiar cola, liberar nodos, reiniciar consumos

---

## 🔒 **Mecanismos de Sincronización**

| Patrón | Implementación | Recurso Protegido |
|--------|---------------|-------------------|
| **Exclusión Mutua** | `sem_wait(&mutex_nodo)` | Nodos individuales |
| **Exclusión Mutua** | `sem_wait(&mutex_consumo)` | Consumo por nodo |
| **Productor-Consumidor** | `lleno` / `vacio` | Cola de solicitudes |
| **Lectores-Escritores** | `mutex_lectores` / `escritor` | Bloques horarios |
| **Señalización** | `mutex_auditoria` | Notificación al auditor |

### 📊 **Semáforos Utilizados**

| Semáforo | Inicial | Propósito |
|----------|---------|-----------|
| `mutex_nodo[i]` | 1 | Proteger operaciones sobre nodo i |
| `mutex_consumo[i]` | 1 | Proteger actualizaciones de consumo |
| `mutex_lectores[h]` | 1 | Proteger contador de lectores |
| `escritor[h]` | 1 | Exclusión para escritores |
| `mutex_cola` | 1 | Exclusión para cola |
| `lleno` | MAX_SOLICITUDES | Espacios disponibles en cola |
| `vacio` | 0 | Elementos disponibles en cola |
| `mutex_auditoria` | 1 | Notificaciones al auditor |
| `mutex_monitor` | 1 | Proteger salida del monitor |
| `mutex_global` | 1 | Proteger variables globales |

---

## 📈 **Resultados a Contabilizar**

Según el enunciado del problema, la simulación de un mes debe reportar:

1. ✅ **Metros cúbicos procesados** - `sistema->total_metros_cubicos`
2. ✅ **Amonestaciones digitales** - `sistema->total_amonestaciones`
3. ✅ **Consumos críticos vs estándar** - `total_consumos_criticos` / `total_consumos_estandar`
4. ✅ **Eficiencia del sistema** - `(nodos_ocupados * 100) / (TOTAL_HORAS * NUM_NODOS)`
5. ✅ **Consumos validados vs no justificados** - Estadísticas del auditor
6. ✅ **Multas generadas** - `auditor.multas_generadas`

---

## 🚀 **Compilación y Ejecución**

### Requisitos
- Compilador GCC
- Biblioteca pthread
- Sistema Unix/Linux o WSL en Windows

### Compilación

```bash
# Usando make
make clean
make

# O compilación manual
gcc -Wall -Wextra -std=c99 -pthread *.c -o ECOFLOW
