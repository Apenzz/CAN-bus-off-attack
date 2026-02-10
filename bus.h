#include <stdint.h>
#include <stdbool.h>

typedef struct ECU ECU;
typedef struct Frame Frame;

typedef struct CANBus {
    ECU **nodes; /* connected ecus */
    uint8_t node_count;
    uint8_t max_nodes;
    bool is_idle;
    bool collision_detected; /* a collision happens only when more ECUs try to send at the same time with the same IDs and there's a bit error in the DLC or data field */
    Frame *winning_msg; /* winning can message for the current simulation tick */
} CANBus;

CANBus* bus_init(int max_nodes);

void destroy_bus(CANBus *bus);

int register_ecu(ECU *ecu, CANBus *bus);
