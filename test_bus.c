#include <assert.h>
#include <stddef.h>

#include "test_bus.h"
#include "bus.h"
#include "ecu.h"



static void test_bus_process_tick_idle() {
    CANBus *bus = bus_init(2);
    ECU *ecu = ecu_init();
    register_ecu(ecu, bus);

    bus_process_tick(bus);

    assert(bus->is_idle == true);
    assert(bus->winning_msg == NULL);
    assert(bus->collision_detected == false);

    destroy_bus(bus);
}

static void test_bus_process_tick_single_transmission() {
    CANBus *bus = bus_init(2);
    ECU *ecu = ecu_init();
    register_ecu(ecu, bus);

    add_frame_to_ecu(0x100, 1, (uint8_t[]){0xFF}, 10, ecu);
    send(ecu, ecu->msg_list);

    bus_process_tick(bus);

    assert(bus->is_idle == false);
    assert(bus->winning_msg == ecu->msg_list);
    assert(bus->collision_detected == false);

    destroy_bus(bus);
}

static void test_bus_process_tick_arbitration() {
    CANBus *bus = bus_init(2);
    ECU *ecu1 = ecu_init();
    ECU *ecu2 = ecu_init();
    register_ecu(ecu1, bus);
    register_ecu(ecu2, bus);

    add_frame_to_ecu(0x200, 1, (uint8_t[]){0xAA}, 10, ecu1);
    add_frame_to_ecu(0x100, 1, (uint8_t[]){0xBB}, 10, ecu2);

    send(ecu1, ecu1->msg_list);
    send(ecu2, ecu2->msg_list);

    bus_process_tick(bus);

    assert(bus->winning_msg == ecu2->msg_list);
    assert(bus->collision_detected == false);

    destroy_bus(bus);
}

static void test_bus_process_tick_collision() {
    CANBus *bus = bus_init(2);
    ECU *victim = ecu_init();
    ECU *attacker = ecu_init();
    register_ecu(victim, bus);
    register_ecu(attacker, bus);

    add_frame_to_ecu(0x100, 1, (uint8_t[]){0xFF}, 10, victim);
    add_frame_to_ecu(0x100, 1, (uint8_t[]){0x00}, 10, attacker);

    send(victim, victim->msg_list);
    send(attacker, attacker->msg_list);

    bus_process_tick(bus);

    assert(bus->winning_msg->id == 0x100);
    assert(bus->collision_detected == true);

    destroy_bus(bus);
}

static void test_bus_process_tick_same_frame_no_collision() {
    CANBus *bus = bus_init(2);
    ECU *ecu1 = ecu_init();
    ECU *ecu2 = ecu_init();
    register_ecu(ecu1, bus);
    register_ecu(ecu2, bus);

    add_frame_to_ecu(0x100, 1, (uint8_t[]){0xFF}, 10, ecu1);
    add_frame_to_ecu(0x100, 1, (uint8_t[]){0xFF}, 10, ecu2);

    send(ecu1, ecu1->msg_list);
    send(ecu2, ecu2->msg_list);

    bus_process_tick(bus);

    assert(bus->collision_detected == false);

    destroy_bus(bus);
}

static void test_register_ecu() {
    ECU *ecu = ecu_init();
    CANBus *bus = bus_init(2);
    register_ecu(ecu, bus);
    assert(bus->nodes[0] == ecu);
    assert(ecu->bus == bus);
}

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
    test_register_ecu();

    test_bus_process_tick_idle();
    test_bus_process_tick_single_transmission();
    test_bus_process_tick_arbitration();
    test_bus_process_tick_collision();
    test_bus_process_tick_same_frame_no_collision();
}
