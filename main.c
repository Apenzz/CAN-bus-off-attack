#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

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

ECU* ecu_init() {
    ECU *ecu = (ECU*)malloc(sizeof(*ecu));
    if (!ecu) return NULL;

    ecu->state = ERROR_ACTIVE;
    ecu->is_transmitting = false;
    return ecu;
}

int main() {
    printf("Beginning of simulation\n");
    // TODO
    printf("End of the simulation\n");
}
