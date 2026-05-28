#ifndef WINDA_UTILS_H
#define WINDA_UTILS_H

#include <pthread.h>
#include <semaphore.h>

// Definicja możliwych kierunków jazdy windy
typedef enum { 
    DIR_STOP = 0, 
    DIR_UP = 1, 
    DIR_DOWN = -1 
} Direction;

// Struktura przechowująca stan pojedynczej windy
typedef struct {
    int id;
    int current_floor;
    int passengers;
    int capacity;
    Direction dir;
} Elevator;

// Struktura przechowująca globalny stan całego akademika
typedef struct {
    int num_elevators;
    int num_floors;
    int num_students;
    int elevator_capacity;
    
    int active_students;
    int *students_waiting_on_floor;
    Elevator *elevators;
    
    pthread_mutex_t system_mutex;
    
    // Narzędzia dla wariantu ze zmiennymi warunkowymi
    pthread_cond_t system_cond;

    // Narzędzia dla wariantu z semaforami
    sem_t system_sem;
    sem_t *capacity_sems;
    
} SystemState;

extern SystemState sys;
void print_system_state(int active_floor, const char* event_msg);

#endif