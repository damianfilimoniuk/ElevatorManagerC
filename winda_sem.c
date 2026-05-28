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

    // 1. CZEKANIE NA WINDĘ
    Elevator* my_elevator = NULL;
    while (my_elevator == NULL) {
        for (int i = 0; i < sys.num_elevators; i++) {
            Elevator* el = &sys.elevators[i];
            
            // Winda jest na naszym piętrze i jedzie w naszą stronę
            if (el->current_floor == start_floor && (el->dir == my_dir || el->dir == DIR_STOP)) {
                
                // KONTROLA ZASOBU SEMAFOREM: Próbujemy zabrać "token" wolnego miejsca.
                if (sem_trywait(&sys.capacity_sems[i]) == 0) {
                    my_elevator = el; // Sukces, zabraliśmy token, wchodzimy!
                    break;
                }
            }
        }
        
        if (my_elevator == NULL) {
            pthread_mutex_unlock(&sys.system_mutex);
            // Czekamy na impuls od windy używając semafora
            sem_wait(&sys.system_sem);
            pthread_mutex_lock(&sys.system_mutex);
        }
    }

    // 2. WSIADANIE
    sys.students_waiting_on_floor[start_floor]--;
    my_elevator->passengers++;
    my_elevator->dir = my_dir; 
    
    snprintf(msg, sizeof(msg), "Student %d wsiada do W%d na P%d", id, my_elevator->id, start_floor);
    print_system_state(start_floor, msg);

    // 3. JAZDA
    while (my_elevator->current_floor != dest_floor) {
        pthread_mutex_unlock(&sys.system_mutex);
        sem_wait(&sys.system_sem);
        pthread_mutex_lock(&sys.system_mutex);
    }

    // 4. WYSIADANIE
    my_elevator->passengers--;
    sys.active_students--; 
    
    // ODDANIE ZASOBU: Wyszliśmy z windy, więc oddajemy token pojemności
    sem_post(&sys.capacity_sems[my_elevator->id - 1]);
    
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
        usleep(1500000); 

        pthread_mutex_lock(&sys.system_mutex);

        if (sys.active_students == 0) {
            // Obudzenie reszty, by mogli zakończyć wątki
            for(int i=0; i<sys.num_students; i++) sem_post(&sys.system_sem);
            pthread_mutex_unlock(&sys.system_mutex);
            break;
        }

        // --- INTELIGENTNY ALGORYTM RUCHU (SSTF / LOOK) ---
        if (me->passengers == 0) {
            // Winda jest pusta - skanuje budynek w poszukiwaniu najbliższego zgłoszenia
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
                me->dir = DIR_STOP; // Brak zgłoszeń, winda śpi
            } else if (target_floor > me->current_floor) {
                me->dir = DIR_UP;   // Wezwanie jest wyżej
            } else if (target_floor < me->current_floor) {
                me->dir = DIR_DOWN; // Wezwanie jest niżej
            } else {
                me->dir = DIR_STOP; // Jesteśmy na piętrze ze zgłoszeniem
            }
        } 
        
        // Zabezpieczenie fizyczne przed wyjazdem poza budynek
        if (me->dir == DIR_UP && me->current_floor == sys.num_floors - 1) me->dir = DIR_DOWN;
        else if (me->dir == DIR_DOWN && me->current_floor == 0) me->dir = DIR_UP;

        // Przemieszczenie
        me->current_floor += me->dir;

        char msg[64];
        snprintf(msg, sizeof(msg), "Winda %d przyjeżdża na P%d", me->id, me->current_floor);
        print_system_state(me->current_floor, msg);

        // Powiadomienie studentów impulsami semafora
        for(int i = 0; i < sys.num_students; i++) {
            sem_post(&sys.system_sem);
        }

        pthread_mutex_unlock(&sys.system_mutex);
    }
    return NULL;
}

// Funkcja głowna 2 wariantu
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
    
    // Inicjalizacja Semaforów nienazwanych (wersja dla Linuxa)
    sem_init(&sys.system_sem, 0, 0); 
    sys.capacity_sems = malloc(sys.num_elevators * sizeof(sem_t));
    for (int i = 0; i < sys.num_elevators; i++) {
        sem_init(&sys.capacity_sems[i], 0, sys.elevator_capacity); 
        
        sys.elevators[i].id = i + 1;
        sys.elevators[i].current_floor = 0;
        sys.elevators[i].passengers = 0;
        sys.elevators[i].capacity = sys.elevator_capacity;
        sys.elevators[i].dir = DIR_STOP;
    }

    printf("--- START SYMULACJI (Wariant: SEMAFORY + INTELIGENTNY ALGORYTM) ---\n\n");

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
        usleep(rand() % 2000000); 
    }

    for (int i = 0; i < sys.num_students; i++) pthread_join(student_threads[i], NULL);
    for (int i = 0; i < sys.num_elevators; i++) pthread_join(elevator_threads[i], NULL);

    printf("\n--- SYMULACJA ZAKOŃCZONA SUKCESEM ---\n");

    // Sprzątanie po semaforach
    sem_destroy(&sys.system_sem);
    for (int i = 0; i < sys.num_elevators; i++) {
        sem_destroy(&sys.capacity_sems[i]);
    }
    
    free(sys.capacity_sems);
    free(sys.students_waiting_on_floor);
    free(sys.elevators);
    free(elevator_threads);
    free(student_threads);
    pthread_mutex_destroy(&sys.system_mutex);

    return 0;
}
