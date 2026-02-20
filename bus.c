#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "ecu.h"
#include "frame.h"

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
    bus->winning_msg = NULL;
    return bus;
}

void destroy_bus(CANBus *bus) {
    if (!bus) return;
    
    for (int i = 0; i < bus->node_count; i++) {
        destroy_ecu(bus->nodes[i]); 
    }
    free(bus->nodes);
    free(bus);
}

int register_ecu(ECU* ecu, CANBus *bus) {
    if (!ecu || !bus) return -1;

    if (bus->node_count < bus->max_nodes) {
        bus->nodes[bus->node_count++] = ecu; /* register ecu to bus */
        ecu->bus = bus; /* ecu has referenct to the bus its connected to */
    }
    return 0;
}

void bus_process_tick(CANBus *bus) {
    if (!bus) return;
    
    /* Reset the bus state for this tick */
    bus->collision_detected = false;
    bus->winning_msg = NULL;
    bus->is_idle = true;

    /* Find all the transmitting ECUs and determine the winner (lowest ID) */
    uint32_t lowest_id = 0xFFFFFFFF;
    Frame *winning_frame = NULL;
    int transmitting_count = 0;

    for (int i = 0; i < bus->node_count; i++) {
        ECU *node = bus->nodes[i];
        if (node->is_transmitting && node->current_msg) {
            transmitting_count++;

            if (node->current_msg->id < lowest_id) {
                lowest_id = node->current_msg->id;
                winning_frame = node->current_msg;
            }
        }
    }

    /* If no one is transmitting, bus remains idle */
    if (transmitting_count == 0) {
        return;
    }
    
    /* Bus is active, set the winner */
    bus->is_idle = false;
    bus->winning_msg = winning_frame;

    /* Check for collisions (same ID but different DLC or data) */
    for (int i = 0; i < bus->node_count; i++) {
        ECU *node = bus->nodes[i];
        if (!node->is_transmitting || !node->current_msg) continue;

        /* Only check nodes that are transmitting the winning ID */
        if (node->current_msg->id == lowest_id) {
            /* Compare with the winning frame */
            if (node->current_msg != winning_frame) {
                /* Check for bit errors */
                if (node->current_msg->dlc != winning_frame->dlc ||
                        memcmp(node->current_msg->data, winning_frame->data, winning_frame->dlc) != 0) {
                    bus->collision_detected = true;
                    break;
                }
            }
        }
    }
}
