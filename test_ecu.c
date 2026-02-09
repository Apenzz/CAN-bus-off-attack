#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "test_ecu.h"
#include "ecu.h"

static void test_ecu_init_with_correct_parameters() {
    ECU *ecu = ecu_init();   
    assert(ecu->is_transmitting == false);
    assert(ecu->state == ERROR_ACTIVE);
    assert(ecu->bus == NULL);
    assert(ecu->msg_list == NULL);
    assert(ecu->msg_count == 0);
    assert(ecu->periods == NULL);
}

void run_ecu_tests() {
    test_ecu_init_with_correct_parameters();
}
