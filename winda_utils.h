#include <pthread.h>
#include <semaphore.h>
#ifndef WINDA_UTILS_H
#define WINDA_UTILS_H

typedef enum { 
	DIR_STOP = 0, 
	DIR_UP = 1, 
	DIR_DOWN = -1 
} Direction;

typedef struct {
	int id;
	int current_floor;
	int passengers;
	int capacity;
	Direction dir;
} Elevator;

typedef struct {
	int num_elevators;
	int num_floors;
	int *students_waiting_on_floor;
	Elevator *elevators;
	pthread_mutex_t hall_mutex;
	
	pthread_cond_t hall_cond;

	sem_t *floor_sems;
	sem_t *elevator_sems; 
} ResidenceHall;

extern ResidenceHall hall;

void print_system_state(int active_floor, const char* event_msg);

#endif
