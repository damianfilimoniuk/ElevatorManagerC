#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "winda_utils.h"

void* elevator_thread(void* arg) {
    int id = *(int*)arg; // Odbieranie przypisanego ID windy
    free(arg);

    while (1) {
        usleep(500000); // Czas przejazdu między piętrami

        pthread_mutex_lock(&hall.hall_mutex); // Zablokowanie mutex'a

        Elevator* me = &hall.elevators[id];

	// Algorytm ruchu
        if (me->passengers == 0) {
            int target_floor = -1;
            int min_dist = hall.num_floors + 1;
	    
	    // Poszukiwanie najbliższego piętra z czekającymi studentami
            for (int i = 0; i < hall.num_floors; i++) {
                if (hall.students_waiting_on_floor[i] > 0) {
                    int dist = abs(me->current_floor - i);
                    if (dist < min_dist) {
                        min_dist = dist;
                        target_floor = i;
                    }
                }
            }
	    // Ustalenie kierunku jazdy na podstawie ustlonego celu
            if (target_floor == -1) {
                me->dir = DIR_STOP;
            } else if (target_floor > me->current_floor) {
                me->dir = DIR_UP;
            } else if (target_floor < me->current_floor) {
                me->dir = DIR_DOWN;
            } else {
                me->dir = DIR_STOP;
            }
        }
	
	// Zabezpieczenie przed wyjazdem poza budynek
        if (me->dir == DIR_UP && me->current_floor == hall.num_floors - 1)
            me->dir = DIR_DOWN;
        else if (me->dir == DIR_DOWN && me->current_floor == 0)
            me->dir = DIR_UP;
	
	// Przemieszczenie windy
        me->current_floor += me->dir;

        char msg[64];
        sprintf(msg, "Winda P%d", me->current_floor);
        print_system_state(me->current_floor, msg);

        pthread_cond_broadcast(&hall.hall_cond); // Poinformowanie śpiących watków studentóœ
        pthread_mutex_unlock(&hall.hall_mutex); // Odblokowanie mutexy
    }
    return NULL;
}

void* student_thread(void* arg) {
    pthread_detach(pthread_self());
    
    int id = *(int*)arg;
    free(arg);

    // Losowanie trasy studenta
    int start_floor = rand() % hall.num_floors;
    int dest_floor = rand() % hall.num_floors;
    while (start_floor == dest_floor) dest_floor = rand() % hall.num_floors;

    Direction my_dir = (dest_floor > start_floor) ? DIR_UP : DIR_DOWN;

    pthread_mutex_lock(&hall.hall_mutex);

    // Zgłoszenie się na piętrze
    hall.students_waiting_on_floor[start_floor]++;
    char msg[64];
    sprintf(msg, "Student %d czeka na P%d (chce P%d)", id, start_floor,
            dest_floor);
    print_system_state(start_floor, msg);

    // Czekanie na odpowiednią windę
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
            pthread_cond_wait(&hall.hall_cond, &hall.hall_mutex);
        }
    }
	
	
    // Wsidanie do windy
    hall.students_waiting_on_floor[start_floor]--;
    my_elevator->passengers++;
    my_elevator->dir = my_dir;

    sprintf(msg, "Student %d wsiada do W%d na P%d", id, my_elevator->id,
            start_floor);
    print_system_state(start_floor, msg);
    
    // Podróź
    while (my_elevator->current_floor != dest_floor) {
        pthread_cond_wait(&hall.hall_cond, &hall.hall_mutex);
    }
    
    
    // Wysiadanie z winydy
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
	
    // Odbieranie argumentów
    if (argc != 4) {
        fprintf(stderr,
                "Użycie: %s <liczba_wind> <liczba_pieter> <pojemnosc_windy>\n",
                argv[0]);
        exit(1);
    }

    hall.num_elevators = atoi(argv[1]);
    hall.num_floors = atoi(argv[2]);
    int elevator_capacity = atoi(argv[3]);

    // Alokacja pamięci dla budynku
    hall.elevators = (Elevator*)malloc(hall.num_elevators * sizeof(Elevator));
    hall.students_waiting_on_floor = (int*)calloc(hall.num_floors, sizeof(int));

    // Inicjlaiacja parametrów wind
    for (int i = 0; i < hall.num_elevators; i++) {
        hall.elevators[i].id = i + 1;
        hall.elevators[i].current_floor = 0;
        hall.elevators[i].passengers = 0;
        hall.elevators[i].capacity = elevator_capacity;
        hall.elevators[i].dir = DIR_UP;
    }
    
    // Inicjalizacja narzędzi synchronizacji
    pthread_mutex_init(&hall.hall_mutex, NULL);
    pthread_cond_init(&hall.hall_cond, NULL);

    printf("--- START SYMULACJI ---\n");

    // Inicjalizacja wątków wind
    pthread_t* elevator_threads =
        malloc(hall.num_elevators * sizeof(pthread_t));
    for (int i = 0; i < hall.num_elevators; i++) {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&elevator_threads[i], NULL, elevator_thread, id);
    }
    
    // Nieskończone generowanie studentów
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
