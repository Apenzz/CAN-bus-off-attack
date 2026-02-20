#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

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
    assert(ecu->is_attacker == false);

    destroy_ecu(ecu);
}

static void test_check_transmission_outcome_collision() {
    AttackFixture *f = create_attack_fixture();

    /* Both send same ID with different data */
    add_frame_to_ecu(0x100, 1, (uint8_t[]){0xFF}, 10, f->victim);
    add_frame_to_ecu(0x100, 1, (uint8_t[]){0x00}, 10, f->attacker);
    
    set_as_attacker(f->attacker);

    send(f->victim, f->victim->msg_list);
    send(f->attacker, f->attacker->msg_list);

    /* Process bus, collision should be detected */
    bus_process_tick(f->bus);

    assert(f->bus->collision_detected == true);

    /* Both ECUs increment TEC */
    check_transmission_outcome(f->victim);
    check_transmission_outcome(f->attacker);

    assert(f->victim->tec == 8);
    assert(f->attacker->tec == 8);

    /* attacker can reset TEC */
    attacker_reset_tec(f->attacker); //TODO: FIND THE FRICKING ERROR!!!!
    printf("ATTAcker TEC: %d\n", f->attacker->tec);
    assert(f->attacker->tec == 0);
    assert(f->victim->tec == 8);

    destroy_attack_fixture(f);
}

static void test_check_transmission_outcome_successful() {
    SimpleFixture *f = create_simple_fixture();

    add_frame_to_ecu(0x100, 1, (uint8_t[]){0xFF}, 10, f->ecu);
    f->ecu->tec = 10; /* start with some errors */

    send(f->ecu, f->ecu->msg_list);
    bus_process_tick(f->bus);

    assert(f->bus->collision_detected == false);

    check_transmission_outcome(f->ecu);

    assert(f->ecu->tec == 9);
    assert(f->ecu->is_transmitting == false);

    destroy_simple_fixture(f);
}

static void test_check_transmission_outcome_lost_arbitration() {
    AttackFixture *f = create_attack_fixture();

    /* Victim sends higher ID */
    add_frame_to_ecu(0x200, 1, (uint8_t[]){0xFF}, 10, f->victim);
    /* Attacker sends lower ID */
    add_frame_to_ecu(0x100, 1, (uint8_t[]){0xFF}, 10, f->attacker);

    f->victim->tec = 10;

    send(f->victim, f->victim->msg_list);
    send(f->attacker, f->attacker->msg_list);

    bus_process_tick(f->bus);

    assert(f->bus->winning_msg->id == 0x100);

    check_transmission_outcome(f->victim);
    check_transmission_outcome(f->attacker);

    assert(f->victim->tec == 10);
    assert(f->attacker->tec == 0);

    destroy_attack_fixture(f);
}

static void test_ecu_state_transitions() {
    ECU *ecu = ecu_init();

    /* error active initially */
    assert(ecu->state == ERROR_ACTIVE);

    /* simulate errors to reach ERROR_PASSIVE */
    ecu->tec = 128;
    send(ecu, &(Frame){.id = 0x100, .dlc = 1});
    ecu->is_transmitting = false; /* reset for test */
    
    ecu->is_transmitting = true;
    CANBus *bus = bus_init(2);
    register_ecu(ecu, bus);
    bus->winning_msg = &(Frame){.id = 0x100};
    bus->collision_detected = false;
    check_transmission_outcome(ecu);

    assert(ecu->state == ERROR_PASSIVE);

    /* simulate more errors to reach bus_off */
    ecu->tec = 256;
    ecu->is_transmitting = true;
    check_transmission_outcome(ecu);

    assert(ecu->state == BUS_OFF);
    
    destroy_bus(bus);
}

static void test_attacker_flag() {
    ECU *normal_ecu = ecu_init();
    ECU *attacker_ecu = ecu_init();

    /* Initialy both are normal */
    assert(normal_ecu->is_attacker == false);
    assert(attacker_ecu->is_attacker == false);

    set_as_attacker(attacker_ecu);

    assert(normal_ecu->is_attacker == false);
    assert(attacker_ecu->is_attacker == true);

    destroy_ecu(normal_ecu);
    destroy_ecu(attacker_ecu);
}

static void test_attacker_tec_reset() {
    ECU *normal_ecu = ecu_init();
    ECU *attacker_ecu = ecu_init();
    set_as_attacker(attacker_ecu);

    normal_ecu->tec = 100;
    attacker_ecu->tec = 100;

    attacker_reset_tec(normal_ecu);
    attacker_reset_tec(attacker_ecu);

    assert(normal_ecu->tec == 100);
    assert(attacker_ecu->tec == 0);

    destroy_ecu(normal_ecu);
    destroy_ecu(attacker_ecu);
}

void run_ecu_tests() {
    test_ecu_init_with_correct_parameters();
    test_add_can_message();
    test_send();
    test_listen();
    test_check_transmission_outcome_collision();
    test_check_transmission_outcome_successful();
    test_check_transmission_outcome_lost_arbitration();
    test_ecu_state_transitions();
    test_attacker_flag();
    test_attacker_tec_reset();
}
