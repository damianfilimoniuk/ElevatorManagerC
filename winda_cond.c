#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "winda_utils.h"

// Wątek studenta
void* student_thread(void* arg) {
    int id = *(int*)arg;
    free(arg);

    int start_floor = rand() % sys.num_floors;
    int dest_floor = rand() % sys.num_floors;
    while(start_floor == dest_floor) { dest_floor = rand() % sys.num_floors; }
    Direction my_dir = (dest_floor > start_floor) ? DIR_UP : DIR_DOWN;

    pthread_mutex_lock(&sys.system_mutex);
    sys.students_waiting_on_floor[start_floor]++;
    
    char msg[64];
    snprintf(msg, sizeof(msg), "Student %d czeka na P%d (chce na P%d)", id, start_floor, dest_floor);
    print_system_state(start_floor, msg);

    // Czekanie na odpowiednią windę
    Elevator* my_elevator = NULL;
    while (my_elevator == NULL) {
        for (int i = 0; i < sys.num_elevators; i++) {
            Elevator* el = &sys.elevators[i];
            if (el->current_floor == start_floor && el->passengers < el->capacity) {
                if (el->dir == my_dir || el->dir == DIR_STOP) {
                    my_elevator = el;
                    break;
                }
            }
        }
        if (my_elevator == NULL) {
            pthread_cond_wait(&sys.system_cond, &sys.system_mutex);
        }
    }

    // Wsiadanie
    sys.students_waiting_on_floor[start_floor]--;
    my_elevator->passengers++;
    my_elevator->dir = my_dir; 
    
    snprintf(msg, sizeof(msg), "Student %d wsiada do W%d na P%d", id, my_elevator->id, start_floor);
    print_system_state(start_floor, msg);

    // Jazda
    while (my_elevator->current_floor != dest_floor) {
        pthread_cond_wait(&sys.system_cond, &sys.system_mutex);
    }

    // Wysiadanie
    my_elevator->passengers--;
    sys.active_students--; 
    
    snprintf(msg, sizeof(msg), "Student %d wysiada z W%d na P%d", id, my_elevator->id, dest_floor);
    print_system_state(dest_floor, msg);

    if (my_elevator->passengers == 0) my_elevator->dir = DIR_STOP;

    pthread_mutex_unlock(&sys.system_mutex);
    return NULL;
}

// Wątek windy
void* elevator_thread(void* arg) {
    int id = *(int*)arg;
    free(arg);
    Elevator* me = &sys.elevators[id];

    while (1) {
        usleep(1500000); // 1.5 sekundy czasu przejazdu

        pthread_mutex_lock(&sys.system_mutex);

        if (sys.active_students == 0) {
            pthread_mutex_unlock(&sys.system_mutex);
            break;
        }

        if (me->passengers == 0) {
            int target_floor = -1;
            int min_dist = sys.num_floors + 1;
            
            for (int i = 0; i < sys.num_floors; i++) {
                if (sys.students_waiting_on_floor[i] > 0) {
                    int dist = abs(me->current_floor - i);
                    if (dist < min_dist) {
                        min_dist = dist;
                        target_floor = i;
                    }
                }
            }

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
        
        if (me->dir == DIR_UP && me->current_floor == sys.num_floors - 1) me->dir = DIR_DOWN;
        else if (me->dir == DIR_DOWN && me->current_floor == 0) me->dir = DIR_UP;

        me->current_floor += me->dir;

        char msg[64];
        snprintf(msg, sizeof(msg), "Winda %d przyjeżdża na P%d", me->id, me->current_floor);
        print_system_state(me->current_floor, msg);

        pthread_cond_broadcast(&sys.system_cond);
        pthread_mutex_unlock(&sys.system_mutex);
    }
    return NULL;
}

// Funkcja głowna programu
int main(int argc, char *argv[]) {
    srand(time(NULL));

    if (argc != 5) {
        fprintf(stderr, "Użycie: %s <windy> <pietra> <studenci> <pojemnosc>\n", argv[0]);
        exit(1);
    }

    sys.num_elevators = atoi(argv[1]);
    sys.num_floors = atoi(argv[2]);
    sys.num_students = atoi(argv[3]);
    sys.elevator_capacity = atoi(argv[4]);
    sys.active_students = sys.num_students;

    sys.students_waiting_on_floor = (int*)calloc(sys.num_floors, sizeof(int));
    sys.elevators = (Elevator*)malloc(sys.num_elevators * sizeof(Elevator));

    pthread_mutex_init(&sys.system_mutex, NULL);
    pthread_cond_init(&sys.system_cond, NULL);

    for (int i = 0; i < sys.num_elevators; i++) {
        sys.elevators[i].id = i + 1;
        sys.elevators[i].current_floor = 0;
        sys.elevators[i].passengers = 0;
        sys.elevators[i].capacity = sys.elevator_capacity;
        sys.elevators[i].dir = DIR_STOP;
    }

    printf("--- START SYMULACJI (Zmienne Warunkowe) ---\n\n");

    pthread_t* elevator_threads = malloc(sys.num_elevators * sizeof(pthread_t));
    for (int i = 0; i < sys.num_elevators; i++) {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&elevator_threads[i], NULL, elevator_thread, id);
    }

    pthread_t* student_threads = malloc(sys.num_students * sizeof(pthread_t));
    for (int i = 0; i < sys.num_students; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&student_threads[i], NULL, student_thread, id);
        usleep(rand() % 2000000); // 0 do 2 sekund
    }

    for (int i = 0; i < sys.num_students; i++) pthread_join(student_threads[i], NULL);
    for (int i = 0; i < sys.num_elevators; i++) pthread_join(elevator_threads[i], NULL);

    printf("\n--- SYMULACJA ZAKOŃCZONA SUKCESEM ---\n");

    free(sys.students_waiting_on_floor);
    free(sys.elevators);
    free(elevator_threads);
    free(student_threads);
    pthread_mutex_destroy(&sys.system_mutex);
    pthread_cond_destroy(&sys.system_cond);

    return 0;
}
