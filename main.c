#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define LOG_FILE "tec_log.csv"

#include "test_ecu.h"
#include "test_bus.h"

#include "ecu.h"
#include "frame.h"
#include "bus.h"

typedef struct {
    uint32_t total_transmissions;
    uint32_t total_collisions;
    uint32_t victim_successful_tx;
    uint32_t attacker_successful_tx;
    uint32_t attacker_tec_resets;
    uint32_t ticks_to_bus_off;
} SimStats;

/* helpers */

static void run_tests(void) {
    printf("Running unit tests...\n");
    run_ecu_tests();
    run_bus_tests();
    printf("All tests passed!\n\n");
}

static void setup_simulation(ECU **victim, ECU **attacker, CANBus **bus) {
    *victim   = ecu_init();
    *attacker = ecu_init();
    set_as_attacker(*attacker);
    *bus = bus_init(2);

    printf("Victim initialization...\n");
    add_frame_to_ecu(0x101, 8, (uint8_t[]){0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}, 10, *victim);
    add_frame_to_ecu(0x102, 1, (uint8_t[]){0x4C}, 20, *victim);
    add_frame_to_ecu(0x103, 2, (uint8_t[]){0xD2,0x09}, 50, *victim);

    register_ecu(*victim,   *bus);
    register_ecu(*attacker, *bus);

    printf("\nAttacker initialization...\n");
    printf("Attacker type: custom CAN controller with TEC manipulation\n");
    printf("Attack strategy: mirror victim IDs with corrupted data, reset TEC after each collision\n\n");
}

static void format_hex_data(char *dest, uint8_t *data, uint8_t dlc) {
    dest[0] = '\0';
    char buf[4];
    for (int i = 0; i < dlc; i++) {
        sprintf(buf, "%02X ", data[i]);
        strcat(dest, buf);
    }
}

static void print_victim_config(ECU *victim) {
    printf("The Victim ECU will send the following periodic messages:\n");
    for (int i = 0; i < victim->msg_count; i++) {
        char hex_str[25];
        format_hex_data(hex_str, victim->msg_list[i].data, victim->msg_list[i].dlc);
        printf("+------------------------------------------------+\n");
        printf("|  ID\t| DLC | \tDATA\t\t| PERIOD |\n");
        printf("+------------------------------------------------+\n");
        printf("| 0x%03X |  %d  | %-24s| %4dms |\n",
                victim->msg_list[i].id,
                victim->msg_list[i].dlc,
                hex_str,
                victim->periods[i]);
        printf("+------------------------------------------------+\n");
    }
    printf("\n");
}

static void victim_tick(ECU *victim, int tick, bool verbose) {
    if (victim->is_transmitting || victim->state == BUS_OFF) return;

    Frame *to_send = NULL;
    uint32_t lowest_id = 0xFFFFFFFF;

    for (int i = 0; i < victim->msg_count; i++) {
        if (tick % victim->periods[i] == 0 && victim->msg_list[i].id < lowest_id) {
            lowest_id = victim->msg_list[i].id;
            to_send = &victim->msg_list[i];
        }
    }

    if (to_send) {
        send(victim, to_send);
        if (verbose)
            printf("[Tick %4d] Victim sending ID 0x%03X, TEC=%d\n", tick, to_send->id, victim->tec);
    }
}

static void attacker_tick(ECU *attacker, ECU *victim, Frame *jam_frame, int tick, bool verbose) {
    if (!victim->is_transmitting || !victim->current_msg || attacker->state == BUS_OFF) return;

    jam_frame->id  = victim->current_msg->id;
    jam_frame->dlc = victim->current_msg->dlc;
    memcpy(jam_frame->data, victim->current_msg->data, jam_frame->dlc);
    jam_frame->data[0] ^= 0xFF;

    send(attacker, jam_frame);

    if (verbose)
        printf("[Tick %4d] Attacker jamming ID 0x%03X, TEC=%d\n", tick, jam_frame->id, attacker->tec);
}

static void process_outcomes(CANBus *bus, ECU *victim, ECU *attacker,
                              bool attacker_was_tx, SimStats *stats, int tick, bool verbose) {
    for (int i = 0; i < bus->node_count; i++) {
        if (bus->nodes[i]->is_transmitting) {
            uint16_t tec_before = bus->nodes[i]->tec;
            check_transmission_outcome(bus->nodes[i]);
            uint16_t tec_after = bus->nodes[i]->tec;

            if (tec_after < tec_before) {
                if      (bus->nodes[i] == victim)   stats->victim_successful_tx++;
                else if (bus->nodes[i] == attacker) stats->attacker_successful_tx++;
            }
        } else {
            listen(bus->nodes[i]);
        }
    }

    if (bus->collision_detected && attacker_was_tx) {
        attacker_reset_tec(attacker);
        stats->attacker_tec_resets++;
        if (verbose)
            printf("[Tick %4d] Attacker reset TEC to 0\n", tick);
    }
}

static void print_simulation_summary(SimStats *stats, ECU *victim, ECU *attacker) {
    printf("\nSIMULATION SUMMARY\n");
    printf("Total Ticks:           %6u\n", stats->ticks_to_bus_off);
    printf("Total Transmissions:   %6u\n", stats->total_transmissions);
    printf("Total Collisions:      %6u\n", stats->total_collisions);
    printf("Victim successful TX:  %6u\n", stats->victim_successful_tx);
    printf("Attacker successful TX:%6u\n", stats->attacker_successful_tx);
    printf("Attacker TEC Resets:   %6u\n", stats->attacker_tec_resets);
    printf("Final victim TEC:      %6u\n", victim->tec);
    printf("Final victim State:    %-20s\n",
            victim->state   == BUS_OFF       ? "BUS_OFF" :
            victim->state   == ERROR_PASSIVE ? "ERROR_PASSIVE" : "ERROR_ACTIVE");
    printf("Final attacker TEC:    %6u\n", attacker->tec);
    printf("Final attacker State:  %-20s\n",
            attacker->state == BUS_OFF       ? "BUS_OFF" :
            attacker->state == ERROR_PASSIVE ? "ERROR_PASSIVE" : "ERROR_ACTIVE");
}

/* simulation loop */

static void run_simulation(CANBus *bus, ECU *victim, ECU *attacker, SimStats *stats) {
    const int  MAX_TICKS = 2000;
    const bool verbose   = false;
    Frame      jam_frame;

    FILE *log = fopen(LOG_FILE, "w");
    if (log) fprintf(log, "tick,victim_tec,attacker_tec,victim_state\n");

    printf("Beginning of simulation...\n");

    for (int tick = 0; tick < MAX_TICKS; tick++) {
        victim_tick(victim, tick, verbose);
        attacker_tick(attacker, victim, &jam_frame, tick, verbose);

        bool attacker_was_tx = attacker->is_transmitting;
        bus_process_tick(bus);

        if (!bus->is_idle)           stats->total_transmissions++;
        if (bus->collision_detected) {
            stats->total_collisions++;
            if (verbose) printf("[Tick %4d] *** COLLISION DETECTED ***\n", tick);
        }

        process_outcomes(bus, victim, attacker, attacker_was_tx, stats, tick, verbose);

        if (log)
            fprintf(log, "%d,%d,%d,%s\n", tick, victim->tec, attacker->tec,
                    victim->state == ERROR_ACTIVE  ? "ERROR_ACTIVE"  :
                    victim->state == ERROR_PASSIVE ? "ERROR_PASSIVE" : "BUS_OFF");

        if (tick > 0 && tick % 100 == 0 && !verbose)
            printf("Tick %4d - Victim TEC: %3d, Attacker TEC: %3d, State: %s, TEC Resets: %d\n",
                    tick, victim->tec, attacker->tec,
                    victim->state == ERROR_ACTIVE  ? "ERROR_ACTIVE"  :
                    victim->state == ERROR_PASSIVE ? "ERROR_PASSIVE" : "BUS_OFF",
                    stats->attacker_tec_resets);

        if (victim->state == BUS_OFF) {
            stats->ticks_to_bus_off = tick;
            printf("\nATTACK SUCCESSFUL!\n");
            printf("Victim entered BUS_OFF state at tick %6d\n", tick);
            if (log) fclose(log);
            return;
        }

        if (attacker->state == BUS_OFF) {
            stats->ticks_to_bus_off = tick;
            printf("UNEXPECTED: Attacker entered BUS_OFF state at tick %d\n", tick);
            printf("This shouldn't happen if TEC reset is working!!\n");
            if (log) fclose(log);
            return;
        }
    }
    if (log) fclose(log);
}

/* entry point */

int main(void) {
    run_tests();

    ECU    *victim, *attacker;
    CANBus *bus;
    setup_simulation(&victim, &attacker, &bus);
    print_victim_config(victim);

    SimStats stats = {0};
    run_simulation(bus, victim, attacker, &stats);
    print_simulation_summary(&stats, victim, attacker);

    destroy_bus(bus);
    printf("End of the simulation\n");
    return 0;
}
