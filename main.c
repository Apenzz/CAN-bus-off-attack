#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "test_ecu.h"
#include "test_bus.h"

#include "ecu.h"
#include "frame.h"

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
    // TODO
    printf("End of the simulation\n");
}
