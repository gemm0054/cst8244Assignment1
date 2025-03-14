#include "../../des_controller/src/des.h"
#include <sys/neutrino.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <controller_pid>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int controller_pid = atoi(argv[1]);
    int coid = ConnectAttach(0, controller_pid, 1, 0, 0);
    if (coid == -1) {
        perror("ConnectAttach failed");
        return EXIT_FAILURE;
    }

    Message msg;
    char input[10];
    while (1) {
        printf(outMessage[0]);
        fflush(stdout);  // Ensure the prompt is shown

        // Use fgets to safely read input
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';  // Remove the newline character from fgets
        if (strcmp(input, inMessage[0]) == 0 || strcmp(input, inMessage[1]) == 0) {
            msg.event = (strcmp(input, inMessage[0]) == 0) ? EVENT_LEFT_SCAN : EVENT_RIGHT_SCAN;
            printf(outMessage[1]);
            fflush(stdout);
            fgets(input, sizeof(input), stdin);
            msg.person_id = atoi(input);  // Convert the input to an integer
        } else if (strcmp(input, inMessage[2]) == 0) {
            msg.event = EVENT_WEIGHT_SCALE;
            printf(outMessage[11]);
            fflush(stdout);
            fgets(input, sizeof(input), stdin);
            msg.weight = atoi(input);  // Convert the input to an integer
        } else if (strcmp(input, inMessage[3]) == 0) {
            msg.event = EVENT_LEFT_OPEN;
        } else if (strcmp(input, inMessage[4]) == 0) {
            msg.event = EVENT_RIGHT_OPEN;
        } else if (strcmp(input, inMessage[5]) == 0) {
            msg.event = EVENT_LEFT_CLOSED;
        } else if (strcmp(input, inMessage[6]) == 0) {
            msg.event = EVENT_RIGHT_CLOSED;
        } else if (strcmp(input, inMessage[7]) == 0) {
            msg.event = EVENT_GUARD_LEFT_UNLOCK;
        } else if (strcmp(input, inMessage[8]) == 0) {
            msg.event = EVENT_GUARD_LEFT_LOCK;
        } else if (strcmp(input, inMessage[9]) == 0) {
            msg.event = EVENT_GUARD_RIGHT_UNLOCK;
        } else if (strcmp(input, inMessage[10]) == 0) {
            msg.event = EVENT_GUARD_RIGHT_LOCK;
        } else if (strcmp(input, inMessage[11]) == 0) {
            msg.event = EVENT_EXIT;
        } else {
            printf("Invalid input, try again.\n");
            continue;
        }

        MsgSend(coid, &msg, sizeof(msg), NULL, 0);
        if (msg.event == EVENT_EXIT) {
        	break;
        }
    }
    	ConnectDetach(coid);
    	return EXIT_SUCCESS;
}
