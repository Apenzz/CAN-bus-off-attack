#ifndef __ECU_H__
#define __ECU_H__

#include <stdbool.h>
#include <stdint.h>

#include "frame.h"
#include "types.h"


typedef enum { ERROR_ACTIVE, ERROR_PASSIVE, BUS_OFF } ECUState;

struct ECU {
    CANBus *bus; /* bus its connected to */
    ECUState state;
    bool is_transmitting;
    uint16_t tec;
    uint16_t rec;

    Frame *msg_list; /* list of periodic can frames */
    uint8_t msg_count; /* number of periodic can frames */
    uint32_t *periods; /* periods of msgs */

    Frame *current_msg; /* current msg it's sending */

    bool is_attacker; /* if true, can manipulate error counters */
};

ECU* ecu_init();

void destroy_ecu(ECU *ecu);

void send(ECU *ecu, Frame *msg);

void listen(ECU *ecu);

void check_transmission_outcome(ECU *ecu); 

/* Attacker specific functions */
void set_as_attacker(ECU *ecu);
void attacker_reset_tec(ECU *ecu);

#endif
