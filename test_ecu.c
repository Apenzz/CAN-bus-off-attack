#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "test_ecu.h"
#include "ecu.h"
#include "frame.h"
#include "bus.h"

typedef struct {
    ECU *ecu;
    CANBus *bus;
} SimpleFixture;

typedef struct {
    ECU *victim;
    ECU *attacker;
    CANBus *bus;
} AttackFixture;

static SimpleFixture* create_simple_fixture() {
    SimpleFixture *f = malloc(sizeof(SimpleFixture));
    f->bus = bus_init(2);
    f->ecu = ecu_init();
    register_ecu(f->ecu, f->bus);
    return f;
}

static void destroy_simple_fixture(SimpleFixture *f) {
    if (!f) return;
    destroy(f->bus);
    free(f);
}

static AttackFixture* create_attack_fixture() {
    AttackFixture *f = malloc(sizeof(AttackFixture));
    f->bus = bus_init(2);
    f->ecu = ecu_init();
    register_ecu(f->victim, f->bus);
    register_ecu(f->attacker, f->bus);
    return f;
}

static void destroy_attack_fixture(AttackFixture *f) {
    if (!f) return;
    destroy_bus(f->bus);
    free(f);
}



static void test_listen() {
    ECU *ecu = ecu_init();
    CANBus *bus = bus_init(0);
    register_ecu(ecu, bus);
    add_frame_to_ecu(0x100, 8, NULL, 10, ecu);
    send(ecu, ecu->msg_list); /* now is_transmitting == true */
    bus->collision_detected = true; /* let's manually set a collision */
    listen(ecu);
    assert(ecu->tec == 8); /*tec should have increased */
    assert(ecu->state == ERROR_ACTIVE); /* should still be in error active mode */
    assert(ecu->is_transmitting == false);
    assert(ecu->current_msg == NULL);
}

static void test_send() {
    ECU *ecu = ecu_init();
    CANBus *bus = bus_init(0);
    register_ecu(ecu, bus);
    add_frame_to_ecu(0x100, 8, NULL, 10, ecu);
    send(ecu, ecu->msg_list);
    assert(ecu->current_msg == ecu->msg_list);
    assert(ecu->is_transmitting == true);
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

void run_ecu_tests() {
    test_ecu_init_with_correct_parameters();
    test_add_can_message();
    test_send();
    test_listen();
}
