#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

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
    destroy_bus(f->bus);
    free(f);
}

static AttackFixture* create_attack_fixture() {
    AttackFixture *f = malloc(sizeof(AttackFixture));
    f->bus = bus_init(2);
    f->victim = ecu_init();
    f->attacker = ecu_init();
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
    SimpleFixture *f = create_simple_fixture();

    add_frame_to_ecu(0x100, 8, NULL, 10, f->ecu);
    send(f->ecu, f->ecu->msg_list); /* now is_transmitting == true */
    f->bus->collision_detected = true; /* let's manually set a collision */
    listen(f->ecu);
    assert(f->ecu->tec == 8); /*tec should have increased */
    assert(f->ecu->state == ERROR_ACTIVE); /* should still be in error active mode */
    assert(f->ecu->is_transmitting == false);
    assert(f->ecu->current_msg == NULL);
}

static void test_send() {
    SimpleFixture *f = create_simple_fixture();

    add_frame_to_ecu(0x100, 8, NULL, 10, f->ecu);
    send(f->ecu, f->ecu->msg_list);

    assert(f->ecu->current_msg == f->ecu->msg_list);
    assert(f->ecu->is_transmitting == true);

    destroy_simple_fixture(f);
}

static void test_add_can_message() {
    SimpleFixture *f = create_simple_fixture();
    uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    add_frame_to_ecu(0x100, 8, data, 10, f->ecu);
    assert(f->ecu->msg_count == 1);
    assert(f->ecu->periods[0] == 10);
    assert(f->ecu->msg_list[0].id == 0x100);
    assert(f->ecu->msg_list[0].dlc == 8);
    assert(memcmp(data, f->ecu->msg_list[0].data, 8) == 0);

    destroy_simple_fixture(f);
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

    destroy_ecu(ecu);
}

static void test_check_transmission_outcome() {
    AttackFixture *f = create_attack_fixture();

    add_frame_to_ecu(0x100, 1, (uint8_t[]){0xFF}, 10, f->victim);
    add_frame_to_ecu(0x100, 1, (uint8_t[]){0x00}, 10, f->attacker);

    send(f->victim, f->victim->msg_list);
    send(f->attacker, f->attacker->msg_list);

    f->bus->collision_detected = true;
    f->bus->winning_msg = f->victim->msg_list;

    check_transmission_outcome(f->victim);

    assert(f->victim->tec == 8);

    destroy_attack_fixture(f);
}

void run_ecu_tests() {
    test_ecu_init_with_correct_parameters();
    test_add_can_message();
    test_send();
    test_listen();
    test_check_transmission_outcome();
}
