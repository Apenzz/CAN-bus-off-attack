#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct CANBus {
    ECU **nodes; /* connected ecus */
    uint8_t node_count;
    bool is_idle;
} CANBus;

int main() {
    printf("Beginning of simulation\n");
    // TODO
    printf("End of the simulation\n");
}
