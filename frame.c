#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "frame.h"
#include "ecu.h"

void add_frame_to_ecu(uint32_t id, uint8_t dlc, uint8_t *data, uint32_t period, ECU *ecu) {
    if (!ecu || dlc > 8) return;
    
    int idx = ecu->msg_count;

    Frame *new_list = realloc(ecu->msg_list, sizeof(Frame) * (idx + 1));
    if (!new_list) return;
    ecu->msg_list = new_list;

    uint32_t *new_periods = realloc(ecu->periods, sizeof(uint32_t) * (idx + 1));
    if (!new_periods) return;
    ecu->periods = new_periods;

    ecu->msg_list[idx].id = id;
    ecu->msg_list[idx].dlc = dlc;
    ecu->periods[idx] = period;

    if (data) {
        memcpy(ecu->msg_list[idx].data, data, dlc);
    } else {
        memset(ecu->msg_list[idx].data, 0, 8);
    }
    ecu->msg_count++;
}
