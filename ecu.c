#include <stdlib.h>
#include <stdbool.h>

#include "ecu.h"

ECU* ecu_init() {
    ECU *ecu = (ECU*)malloc(sizeof(*ecu));
    if (!ecu) return NULL;

    ecu->state = ERROR_ACTIVE;
    ecu->is_transmitting = false;
    return ecu;
}

void destroy_ecu(ECU *ecu) {
    if (!ecu) return;
    free(ecu);
}
