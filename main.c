#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "test_ecu.h"

#include "ecu.h"

typedef struct CANBus {
    ECU **nodes; /* connected ecus */
    uint8_t node_count;
    bool is_idle;
    bool collision_detected; /* if there is a collision in the currect bus tick */
} CANBus;


int main() {
    run_ecu_tests();
    ECU *victim = ecu_init();
    ECU *attacker = ecu_init();
    printf("Beginning of simulation\n");
    // TODO
    printf("End of the simulation\n");
}
