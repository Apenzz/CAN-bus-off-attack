#include <stdlib.h>
#include "bus.h"

CANBus* bus_init(int max_nodes) {
    if (max_nodes < 2 || max_nodes > 255) max_nodes = 2; /* make room for at least 2 nodes: one victim and one attacker */
    CANBus *bus = (CANBus*)malloc(sizeof(*bus)); 
    if (!bus) return NULL;
    bus->nodes = (ECU**)malloc(sizeof(ECU*) * max_nodes);
    if (!bus->nodes) {
        free(bus);
        return NULL;
    }
    bus->node_count = 0;
    bus->max_nodes = max_nodes;
    bus->is_idle = true;
    bus->collision_detected = false;
    return bus;
}

void destroy_bus(CANBus *bus) {
    if (!bus) return;
    
    for (int i = 0; i < bus->node_count; i++) {
        destroy_ecu(bus->nodes[i]); 
    }
    free(bus);
}
