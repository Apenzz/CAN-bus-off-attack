#include <assert.h>
#include <stdbool.h>

#include "test_ecu.h"
#include "ecu.h"

static void test_ecu_init_with_correct_parameters() {
    ECU *ecu = ecu_init();   
    assert(ecu->is_transmitting == false);
    assert(ecu->state == ERROR_ACTIVE);
}

void run_ecu_tests() {
    test_ecu_init_with_correct_parameters();
}
