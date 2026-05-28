#include <stdio.h>
#include "winda_utils.h"

SystemState sys;

// Wyświetlanie stanu systemu
void print_system_state(int active_floor, const char* event_msg) {
    printf("{\"active_floor\": %d, \"msg\": \"%s\", \"waiting\": [", active_floor, event_msg);
    
    // Zrzut stanu kolejek na piętrach
    for (int i = 0; i < sys.num_floors; i++) {
        printf("%d%s", sys.students_waiting_on_floor[i], (i == sys.num_floors - 1) ? "" : ", ");
    }
    
    printf("], \"elevators\": [");
    
    // Zrzut stanu wind
    for (int i = 0; i < sys.num_elevators; i++) {
        printf("{\"id\": %d, \"floor\": %d, \"passengers\": %d, \"capacity\": %d, \"dir\": %d}%s", 
               sys.elevators[i].id, 
               sys.elevators[i].current_floor, 
               sys.elevators[i].passengers, 
               sys.elevators[i].capacity, 
               sys.elevators[i].dir,
               (i == sys.num_elevators - 1) ? "" : ", ");
    }
    printf("]}\n");
    
    fflush(stdout); 
}
