#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum { ERROR_ACTIVE, ERROR_PASSIVE, BUS_OFF } ECUState;

typedef struct ECU {
    ECUState state; 
    bool is_transmitting;
} ECU;

typedef struct CANBus {
    ECU **nodes; /* connected ecus */
    uint8_t node_count;
    bool is_idle;
    bool collision_detected; /* if there is a collision in the currect bus tick */
} CANBus;

int main() {
    printf("Beginning of simulation\n");
    // TODO
    printf("End of the simulation\n");
}
