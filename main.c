#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "test_ecu.h"
#include "test_bus.h"

#include "ecu.h"

int main() {
    run_ecu_tests();
    run_bus_tests();
    ECU *victim = ecu_init();
    ECU *attacker = ecu_init();
    printf("Beginning of simulation\n");
    // TODO
    printf("End of the simulation\n");
}
