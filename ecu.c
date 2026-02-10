#include <stdlib.h>
#include <stdbool.h>

#include "ecu.h"

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
    free(ecu);
}

void send(ECU *ecu, Frame msg) {
    if (!ecu || !ecu->bus || !msg || ecu->state == BUS_OFF) return;

    ecu->current_msg = msg;
    ecu->is_transmitting = true;
}
