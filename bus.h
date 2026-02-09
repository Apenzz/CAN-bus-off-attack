#include <stdint.h>
#include <stdbool.h>

typedef struct ECU ECU;

typedef struct CANBus {
    ECU **nodes; /* connected ecus */
    uint8_t node_count;
    uint8_t max_nodes;
    bool is_idle;
    bool collision_detected; /* if there is a collision in the currect bus tick */
} CANBus;

CANBus* bus_init(int max_nodes);

void destroy_bus(CANBus *bus);

int register_ecu(ECU *ecu, CANBus *bus);
