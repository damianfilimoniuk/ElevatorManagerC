#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "winda_utils.h"

void* elevator_thread(void* arg) {
    int id = *(int*)arg;
    free(arg);

    while (1) {
        usleep(500000);

        pthread_mutex_lock(&hall.hall_mutex);
        Elevator* me = &hall.elevators[id];

        if (me->passengers == 0) {
            int target_floor = -1;
            int min_dist = hall.num_floors + 1;

            for (int i = 0; i < hall.num_floors; i++) {
                if (hall.students_waiting_on_floor[i] > 0) {
                    int dist = abs(me->current_floor - i);
                    if (dist < min_dist) {
                        min_dist = dist;
                        target_floor = i;
                    }
                }
            }

            if (target_floor == -1)
                me->dir = DIR_STOP;
            else if (target_floor > me->current_floor)
                me->dir = DIR_UP;
            else if (target_floor < me->current_floor)
                me->dir = DIR_DOWN;
            else
                me->dir = DIR_STOP;
        }

        if (me->dir == DIR_UP && me->current_floor >= hall.num_floors - 1)
            me->dir = DIR_STOP;
        else if (me->dir == DIR_DOWN && me->current_floor <= 0)
            me->dir = DIR_STOP;

        if (me->dir != DIR_STOP) {
            me->current_floor += me->dir;
            char msg[64];
            sprintf(msg, "Winda W%d jedzie na P%d", me->id, me->current_floor);
            print_system_state(me->current_floor, msg);
        }

	// Ręczne powiadomienie każdego pasującego studentów
        while (sem_trywait(&hall.elevator_sems[id]) == 0);
        for (int i = 0; i < me->passengers; i++) {
            sem_post(&hall.elevator_sems[id]);
        }

        while (sem_trywait(&hall.floor_sems[me->current_floor]) == 0);
        for (int i = 0; i < hall.students_waiting_on_floor[me->current_floor];
             i++) {
            sem_post(&hall.floor_sems[me->current_floor]);
        }

        pthread_mutex_unlock(&hall.hall_mutex);
    }
    return NULL;
}

void* student_thread(void* arg) {
    pthread_detach(pthread_self());
    int id = *(int*)arg;
    free(arg);

    int start_floor = rand() % hall.num_floors;
    int dest_floor = rand() % hall.num_floors;
    while (start_floor == dest_floor) dest_floor = rand() % hall.num_floors;
    Direction my_dir = (dest_floor > start_floor) ? DIR_UP : DIR_DOWN;

    pthread_mutex_lock(&hall.hall_mutex);

    hall.students_waiting_on_floor[start_floor]++;
    char msg[64];
    sprintf(msg, "Student %d czeka na P%d (chce P%d)", id, start_floor,
            dest_floor);
    print_system_state(start_floor, msg);

    Elevator* my_elevator = NULL;

    while (my_elevator == NULL) {
        for (int i = 0; i < hall.num_elevators; i++) {
            Elevator* el = &hall.elevators[i];
            if (el->current_floor == start_floor &&
                el->passengers < el->capacity) {
                if (el->dir == my_dir || el->dir == DIR_STOP) {
                    my_elevator = el;
                    break;
                }
            }
        }

        if (my_elevator == NULL) {
            pthread_mutex_unlock(&hall.hall_mutex);
            sem_wait(&hall.floor_sems[start_floor]);
            pthread_mutex_lock(&hall.hall_mutex);
        }
    }

    hall.students_waiting_on_floor[start_floor]--;
    my_elevator->passengers++;
    my_elevator->dir = my_dir;

    sprintf(msg, "Student %d wsiada do W%d na P%d", id, my_elevator->id,
            start_floor);
    print_system_state(start_floor, msg);

    // Zabezpiecenie przed kradzieżą biletów
    int last_seen_floor = start_floor;
    while (my_elevator->current_floor != dest_floor) {
        pthread_mutex_unlock(&hall.hall_mutex);
        sem_wait(&hall.elevator_sems[my_elevator->id - 1]);
        pthread_mutex_lock(&hall.hall_mutex);

        if (my_elevator->current_floor == last_seen_floor) {
            sem_post(&hall.elevator_sems[my_elevator->id - 1]);

            pthread_mutex_unlock(&hall.hall_mutex);
            usleep(5000);
            pthread_mutex_lock(&hall.hall_mutex);
        } else {
            last_seen_floor = my_elevator->current_floor;
        }
    }

    my_elevator->passengers--;
    if (my_elevator->passengers == 0) my_elevator->dir = DIR_STOP;

    sprintf(msg, "Student %d wysiada z W%d na P%d", id, my_elevator->id,
            dest_floor);
    print_system_state(dest_floor, msg);

    pthread_mutex_unlock(&hall.hall_mutex);
    return NULL;
}

int main(int argc, char* argv[]) {
    srand(time(NULL));

    if (argc != 4) {
        fprintf(stderr,
                "Użycie: %s <liczba_wind> <liczba_pieter> <pojemnosc>\n",
                argv[0]);
        exit(1);
    }

    hall.num_elevators = atoi(argv[1]);
    hall.num_floors = atoi(argv[2]);
    int elevator_capacity = atoi(argv[3]);

    hall.elevators = malloc(hall.num_elevators * sizeof(Elevator));
    hall.students_waiting_on_floor = calloc(hall.num_floors, sizeof(int));

    for (int i = 0; i < hall.num_elevators; i++) {
        hall.elevators[i].id = i + 1;
        hall.elevators[i].current_floor = 0;
        hall.elevators[i].passengers = 0;
        hall.elevators[i].capacity = elevator_capacity;
        hall.elevators[i].dir = DIR_UP;
    }

    pthread_mutex_init(&hall.hall_mutex, NULL);

    // Inicjalizacja tablic semaforów
    hall.floor_sems = malloc(hall.num_floors * sizeof(sem_t));
    for (int i = 0; i < hall.num_floors; i++) {
        sem_init(&hall.floor_sems[i], 0, 0);
    }

    hall.elevator_sems = malloc(hall.num_elevators * sizeof(sem_t));
    for (int i = 0; i < hall.num_elevators; i++) {
        sem_init(&hall.elevator_sems[i], 0, 0);
    }

    printf("--- START SYMULACJI (Wersja: SEMAFORY) ---\n");

    pthread_t* elevator_threads =
        malloc(hall.num_elevators * sizeof(pthread_t));
    for (int i = 0; i < hall.num_elevators; i++) {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&elevator_threads[i], NULL, elevator_thread, id);
    }

    int current_student_id = 1;
    while (1) {
        int* id = malloc(sizeof(int));
        *id = current_student_id++;

        pthread_t st;
        pthread_create(&st, NULL, student_thread, id);
        usleep(rand() % 500000);
    }

    return 0;
}
