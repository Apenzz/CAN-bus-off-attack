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
    
    listen(f->ecu);

    assert(f->ecu->tec == 0);
    assert(f->ecu->state == ERROR_ACTIVE);

    destroy_simple_fixture(f);
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
    printf("ciao!\n");
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

static void test_listen_rec_increments_on_collision() {
    /* 3-node bus: victim and attacker collide, bystander is listening */
    CANBus *bus = bus_init(3);
    ECU *victim    = ecu_init();
    ECU *attacker  = ecu_init();
    ECU *bystander = ecu_init();
    register_ecu(victim,    bus);
    register_ecu(attacker,  bus);
    register_ecu(bystander, bus);

    add_frame_to_ecu(0x100, 1, (uint8_t[]){0xFF}, 10, victim);
    add_frame_to_ecu(0x100, 1, (uint8_t[]){0x00}, 10, attacker);

    send(victim,   victim->msg_list);
    send(attacker, attacker->msg_list);
    bus_process_tick(bus);

    assert(bus->collision_detected == true);

    listen(bystander);

    assert(bystander->rec == 1);
    assert(bystander->state == ERROR_ACTIVE); /* still healthy */

    destroy_bus(bus);
}

static void test_listen_rec_decrements_on_success() {
    CANBus *bus = bus_init(2);
    ECU *sender   = ecu_init();
    ECU *receiver = ecu_init();
    register_ecu(sender,   bus);
    register_ecu(receiver, bus);

    receiver->rec = 10; /* pre-load some errors */

    add_frame_to_ecu(0x100, 1, (uint8_t[]){0xFF}, 10, sender);
    send(sender, sender->msg_list);
    bus_process_tick(bus);

    assert(bus->collision_detected == false);

    listen(receiver);

    assert(receiver->rec == 9);

    destroy_bus(bus);
}

static void test_listen_idle_bus_no_change() {
    CANBus *bus = bus_init(2);
    ECU *ecu = ecu_init();
    register_ecu(ecu, bus);
    ecu->rec = 5;

    bus_process_tick(bus); /* bus stays idle, no one transmitting */

    assert(bus->is_idle == true);

    listen(ecu);

    assert(ecu->rec == 5); /* unchanged */

    destroy_bus(bus);
}

static void test_listen_rec_triggers_error_passive() {
    CANBus *bus = bus_init(3);
    ECU *victim    = ecu_init();
    ECU *attacker  = ecu_init();
    ECU *bystander = ecu_init();
    register_ecu(victim,    bus);
    register_ecu(attacker,  bus);
    register_ecu(bystander, bus);

    bystander->rec = 127; /* one increment away from ERROR_PASSIVE */

    add_frame_to_ecu(0x100, 1, (uint8_t[]){0xFF}, 10, victim);
    add_frame_to_ecu(0x100, 1, (uint8_t[]){0x00}, 10, attacker);

    send(victim,   victim->msg_list);
    send(attacker, attacker->msg_list);
    bus_process_tick(bus);

    assert(bus->collision_detected == true);

    listen(bystander);

    assert(bystander->rec == 128);
    assert(bystander->state == ERROR_PASSIVE);

    destroy_bus(bus);
}

static void test_listen_skipped_for_transmitting_node() {
    CANBus *bus = bus_init(2);
    ECU *ecu1 = ecu_init();
    ECU *ecu2 = ecu_init();
    register_ecu(ecu1, bus);
    register_ecu(ecu2, bus);

    add_frame_to_ecu(0x100, 1, (uint8_t[]){0xFF}, 10, ecu1);
    add_frame_to_ecu(0x100, 1, (uint8_t[]){0x00}, 10, ecu2);

    send(ecu1, ecu1->msg_list);
    send(ecu2, ecu2->msg_list);
    bus_process_tick(bus);

    assert(bus->collision_detected == true);

    /* calling listen() on a transmitting node must be a no-op */
    listen(ecu1);
    listen(ecu2);

    assert(ecu1->rec == 0);
    assert(ecu2->rec == 0);

    destroy_bus(bus);
}

void run_ecu_tests() {
    test_listen();
    test_send();
    test_add_can_message();
    test_ecu_init_with_correct_parameters();
    test_check_transmission_outcome_collision();
    test_check_transmission_outcome_successful();
    test_check_transmission_outcome_lost_arbitration();
    test_attacker_flag();
    test_attacker_tec_reset();
    test_listen_rec_increments_on_collision();
    test_listen_rec_decrements_on_success();
    test_listen_idle_bus_no_change();
    test_listen_rec_triggers_error_passive();
    test_listen_skipped_for_transmitting_node();
}
