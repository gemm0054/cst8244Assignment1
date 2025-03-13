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
        printf("Enter DES input-event (ls = left scan, rs = rightscan, ws weight scale, lo = left open, ro = right open, lc = left closed, rc = right closed, gru = guard right unlock, grl = guard right lock, glu = guard left unlock, gll = guard left lock, exit = exit programs): \n");
        fflush(stdout);  // Ensure the prompt is shown

        // Use fgets to safely read input
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';  // Remove the newline character from fgets
        if (strcmp(input, "ls") == 0 || strcmp(input, "rs") == 0) {
            msg.event = (strcmp(input, "ls") == 0) ? EVENT_LEFT_SCAN : EVENT_RIGHT_SCAN;
            printf("Enter person ID: \n");
            fflush(stdout);
            // Use fgets to get person ID and remove the newline character
            fgets(input, sizeof(input), stdin);
            msg.person_id = atoi(input);  // Convert the input to an integer
        } else if (strcmp(input, "ws") == 0) {
            msg.event = EVENT_WEIGHT_SCALE;
            printf("Enter person's weight: \n");
            fflush(stdout);
            fgets(input, sizeof(input), stdin);
            msg.weight = atoi(input);  // Convert the input to an integer
        } else if (strcmp(input, "lo") == 0) {
            msg.event = EVENT_LEFT_OPEN;
        } else if (strcmp(input, "ro") == 0) {
            msg.event = EVENT_RIGHT_OPEN;
        } else if (strcmp(input, "lc") == 0) {
            msg.event = EVENT_LEFT_CLOSED;
        } else if (strcmp(input, "rc") == 0) {
            msg.event = EVENT_RIGHT_CLOSED;
        } else if (strcmp(input, "glu") == 0) {
            msg.event = EVENT_GUARD_LEFT_UNLOCK;
        } else if (strcmp(input, "gll") == 0) {
            msg.event = EVENT_GUARD_LEFT_LOCK;
        } else if (strcmp(input, "gru") == 0) {
            msg.event = EVENT_GUARD_RIGHT_UNLOCK;
        } else if (strcmp(input, "grl") == 0) {
            msg.event = EVENT_GUARD_RIGHT_LOCK;
        } else if (strcmp(input, "exit") == 0) {
            msg.event = EVENT_EXIT;
            break;
        } else {
            printf("Invalid input, try again.\n");
            continue;
        }

        MsgSend(coid, &msg, sizeof(msg), NULL, 0);
    }

    ConnectDetach(coid);
    return EXIT_SUCCESS;
}
