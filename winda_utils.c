#include <stdio.h>
#include "winda_utils.h"

ResidenceHall hall; // Globalna instancja akademika w pamięci

void print_system_state(int active_floor, const char* event_msg) {
    printf("P%d | ", active_floor);
    // Wypisanie klasycznych logów tekstowych
    for (int i = 0; i < hall.num_elevators; i++) {
        char d = (hall.elevators[i].dir == DIR_UP) ? '^' : ((hall.elevators[i].dir == DIR_DOWN) ? 'v' : 'S');
        printf("W%d: [%d/%d] %c | ", 
               hall.elevators[i].id, 
               hall.elevators[i].passengers, 
               hall.elevators[i].capacity, 
               d);
    }
    printf(" <-- %s\n", event_msg);

    // Zrzut danych w formacie JSON
    printf("{\"active_floor\": %d, \"msg\": \"%s\", \"waiting\": [", active_floor, event_msg);
    for (int i = 0; i < hall.num_floors; i++) {
        printf("%d%s", hall.students_waiting_on_floor[i], (i == hall.num_floors - 1) ? "" : ", ");
    }
    printf("], \"elevators\": [");
    for (int i = 0; i < hall.num_elevators; i++) {
        printf("{\"id\": %d, \"floor\": %d, \"passengers\": %d, \"capacity\": %d, \"dir\": %d}%s", 
               hall.elevators[i].id, 
               hall.elevators[i].current_floor, 
               hall.elevators[i].passengers, 
               hall.elevators[i].capacity, 
               hall.elevators[i].dir,
               (i == hall.num_elevators - 1) ? "" : ", ");
    }
    printf("]}\n");
    
    // Wypchnięcie bufora do terminala/potoku
    fflush(stdout);
}
