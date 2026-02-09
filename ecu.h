#ifndef __ECU_H__
#define __ECU_H__

#include <stdbool.h>
#include <stdint.h>


typedef enum { ERROR_ACTIVE, ERROR_PASSIVE, BUS_OFF } ECUState;
typedef struct CANBus CANBus;
typedef struct Frame Frame;

typedef struct ECU {
    CANBus *bus; /* bus its connected to */
    ECUState state; 
    bool is_transmitting;

    Frame *msg_list; /* list of periodic can frames */
    uint8_t msg_count; /* number of periodic can frames */
    uint32_t *periods; /* periods of msgs */

} ECU;

ECU* ecu_init();

void destroy_ecu(ECU *ecu);

#endif
