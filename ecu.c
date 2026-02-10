#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "ecu.h"
#include "frame.h"
#include "bus.h"

static void update_ecu_state(ECU *ecu) {
    if (ecu->tec > 255) ecu->state = BUS_OFF;
    else if (ecu->tec >= 128 || ecu->rec >= 128) ecu->state = ERROR_PASSIVE;
    else ecu->state = ERROR_ACTIVE;
}

ECU* ecu_init() {
    ECU *ecu = (ECU*)malloc(sizeof(*ecu));
    if (!ecu) return NULL;

    ecu->bus = NULL;
    ecu->state = ERROR_ACTIVE;
    ecu->is_transmitting = false;
    ecu->tec = 0;
    ecu->rec = 0;

    ecu->msg_list = NULL;
    ecu->msg_count = 0;
    ecu->periods = NULL;
    
    ecu->current_msg = NULL;
    return ecu;
}

void destroy_ecu(ECU *ecu) {
    if (!ecu) return;
    free(ecu->msg_list);
    free(ecu->periods);
    free(ecu);
}

void send(ECU *ecu, Frame *msg) {
    if (!ecu || !ecu->bus || !msg || ecu->state == BUS_OFF) return;
    
    ecu->current_msg = msg;
    ecu->is_transmitting = true;
}

void listen(ECU *ecu) {
    if (!ecu || !ecu->bus || ecu->state == BUS_OFF) return;

    if (ecu->is_transmitting && ecu->bus->collision_detected) {
        ecu->tec += 8; 
    } else if (ecu->is_transmitting) {
        if (ecu->tec > 0) ecu->tec--;
    }
    /* update ecu internal state for the next tick */
    update_ecu_state(ecu);
    ecu->is_transmitting = false;
    ecu->current_msg = NULL;
}

void check_transmission_outcome(ECU *ecu) {
    if (!ecu || !ecu->bus || !ecu->is_transmitting) return;

    /* is winning message NULL? */
    if (!ecu->bus->winning_msg) {
        printf("This should never print if bus logic is correct!\n");
        ecu->is_transmitting = false;
        ecu->current_msg = NULL;
        return;
    }
    
    /* Did I win arbitration? */
    if (ecu->current_msg->id == ecu->bus->winning_msg->id) {
        /* Was there a collision? */
        if (ecu->bus->collision_detected) {
            ecu->tec += 8; /* transmission error */
        } else { /* No collision */
            if (ecu->tec > 0) ecu->tec--;
        }
    }
    update_ecu_state(ecu);
    ecu->is_transmitting = false;
    ecu->current_msg = NULL;
} 

