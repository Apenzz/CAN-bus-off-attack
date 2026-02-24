#ifndef __FRAME_H__
#define __FRAME_H__

#include <stdint.h>

#include "types.h"

/* CAN Data Frame
 * For the sake of simplicity this simulation only considers frames that contain the following fields:
 *  - ID: identifier
 *  - DLC: data length code
 *  - Data: the actual data (0 to 8 bytes long)
 *
 */
struct Frame {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
};

void add_frame_to_ecu(uint32_t id, uint8_t dlc, uint8_t *data, uint32_t period, ECU *ecu);

#endif
