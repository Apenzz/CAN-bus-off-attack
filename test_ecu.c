#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "test_ecu.h"
#include "ecu.h"
#include "frame.h"

static void test_add_can_message();
static void test_ecu_init_with_correct_parameters();

void run_ecu_tests() {
    test_ecu_init_with_correct_parameters();
    test_add_can_message();
}

static void test_add_can_message() {
    ECU *ecu = ecu_init();
    uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    add_frame_to_ecu(0x100, 8, data, 10, ecu);
    assert(ecu->msg_count == 1);
    assert(ecu->periods[0] == 10);
    assert(ecu->msg_list[0].id == 0x100);
    assert(ecu->msg_list[0].dlc == 8);
    assert(memcmp(data, ecu->msg_list[0].data, 8) == 0);
}

static void test_ecu_init_with_correct_parameters() {
    ECU *ecu = ecu_init();   
    assert(ecu->is_transmitting == false);
    assert(ecu->state == ERROR_ACTIVE);
    assert(ecu->bus == NULL);
    assert(ecu->tec == 0);
    assert(ecu->rec == 0);
    assert(ecu->msg_list == NULL);
    assert(ecu->msg_count == 0);
    assert(ecu->periods == NULL);
    assert(ecu->current_msg == NULL);
}

