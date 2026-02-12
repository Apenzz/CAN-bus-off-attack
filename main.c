#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "test_ecu.h"
#include "test_bus.h"

#include "ecu.h"
#include "frame.h"
#include "bus.h"

void format_hex_data(char *dest, uint8_t *data, uint8_t dlc) {
    dest[0] = '\0'; /* empty string */
    char buf[4];
    for (int i = 0; i < dlc; i++) {
        sprintf(buf, "%02X ", data[i]);
        strcat(dest, buf);
    }
}

int main() {
    run_ecu_tests();
    run_bus_tests();
    ECU *attacker = ecu_init();

    printf("Victim initialization\n");
    ECU *victim = ecu_init();
    add_frame_to_ecu(0x101, 8, (uint8_t[]){0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}, 10, victim);
    add_frame_to_ecu(0x102, 1, (uint8_t[]){0x4C}, 20, victim);
    add_frame_to_ecu(0x103, 2, (uint8_t[]){0xD2,0x09}, 50, victim);
    CANBus *bus = bus_init(2);
    register_ecu(victim, bus);
    register_ecu(attacker, bus);
    printf("The Victim ecu is going to send the following periodic messages:\n");
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
    printf("Beginning of simulation\n");

    const int simulation_ticks = 2000;
    for (int tick = 0; tick < simulation_ticks; tick++) {
        /* Victim: pick highest priority message that's ready */
        if (!victim->is_transmitting && victim->state != BUS_OFF) {
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
                printf("[Victim] Sending frame at tick %d\n", tick);
                printf("[Victim] TEC = %d\n", victim->tec);
            }
        }

        /* Attacker: react to victim */
        if (victim->is_transmitting && attacker->state != BUS_OFF) { // TODO: FIX
            static Frame jam;
            jam = *victim->current_msg;
            jam.data[0] ^= 0xFF;
            send(attacker, &jam);
            printf("[Attacker] Sending frame at tick %d\n", tick);
            printf("[Attacker] TEC = %d\n", victim->tec);
        }

        bus_process_tick(bus);
        if (bus->collision_detected) {
            printf("Collision detected!!\n");
        }

        for (int i = 0; i < bus->node_count; i++) {
            if (bus->nodes[i]->is_transmitting) {
                check_transmission_outcome(bus->nodes[i]);
            }
        }
    }
    
    printf("End of the simulation\n");
}
