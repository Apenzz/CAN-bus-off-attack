#include <assert.h>

#include "test_bus.h"
#include "bus.h"

static void test_bus_init_with_correct_parameters() {
    CANBus *bus = bus_init(10); 
    assert(bus->node_count == 0);
    assert(bus->max_nodes == 10);
    assert(bus->is_idle == true);
    assert(bus->collision_detected == false);
}

static void test_bus_init_with_max_node_out_of_bounds() {
    CANBus *bus1 = bus_init(256);
    assert(bus1->max_nodes == 2);
    CANBus *bus2 = bus_init(0);
    assert(bus2->max_nodes == 2);
    CANBus *bus3 = bus_init(-4);
    assert(bus3->max_nodes == 2);
}

void run_bus_tests() {
    test_bus_init_with_correct_parameters();
    test_bus_init_with_max_node_out_of_bounds();
}
