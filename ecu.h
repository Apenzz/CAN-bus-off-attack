#ifndef __ECU_H__
#define __ECU_H__

#include <stdbool.h>

typedef enum { ERROR_ACTIVE, ERROR_PASSIVE, BUS_OFF } ECUState;
typedef struct CANBus CANBus;

typedef struct ECU {
    CANBus *bus; /* bus its connected to */
    ECUState state; 
    bool is_transmitting;
} ECU;

ECU* ecu_init();

void destroy_ecu(ECU *ecu);

#endif
